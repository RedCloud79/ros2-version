#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <tf2/LinearMath/Quaternion.h>

#include <yhs_can_interfaces/msg/chassis_info_fb.hpp>
#include <yhs_can_interfaces/msg/ctrl_cmd.hpp>
#include <yhs_can_interfaces/msg/ctrl_fb.hpp>

using namespace std::chrono_literals;

class ControlRobot : public rclcpp::Node
{
public:
  ControlRobot()
  : Node("control_robot_node"),
    last_odom_time_(now())
  {
    declare_parameter<double>("wheel_radius", 0.07);
    declare_parameter<double>("wheel_sign_lf", 1.0);
    declare_parameter<double>("wheel_sign_rf", 1.0);
    declare_parameter<double>("wheel_sign_lr", 1.0);
    declare_parameter<double>("wheel_sign_rr", 1.0);
    declare_parameter<std::string>("odom_topic", "/control_robot/odom");
    declare_parameter<std::string>("odom_frame_id", "odom");
    declare_parameter<std::string>("base_frame_id", "base_footprint");

    wheel_radius_ = get_parameter("wheel_radius").as_double();
    wheel_lf_.sign = get_parameter("wheel_sign_lf").as_double();
    wheel_rf_.sign = get_parameter("wheel_sign_rf").as_double();
    wheel_lr_.sign = get_parameter("wheel_sign_lr").as_double();
    wheel_rr_.sign = get_parameter("wheel_sign_rr").as_double();
    odom_topic_ = get_parameter("odom_topic").as_string();
    odom_frame_id_ = get_parameter("odom_frame_id").as_string();
    base_frame_id_ = get_parameter("base_frame_id").as_string();

    ctrl_cmd_pub_ =
      create_publisher<yhs_can_interfaces::msg::CtrlCmd>("/ctrl_cmd", 10);

    joint_state_pub_ =
      create_publisher<sensor_msgs::msg::JointState>("/joint_states", 10);

    odom_pub_ =
      create_publisher<nav_msgs::msg::Odometry>(odom_topic_, 10);

    cmd_vel_sub_ =
      create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel",
      10,
      std::bind(&ControlRobot::cmdVelCallback, this, std::placeholders::_1));

    chassis_info_sub_ =
      create_subscription<yhs_can_interfaces::msg::ChassisInfoFb>(
      "/chassis_info_fb",
      10,
      std::bind(&ControlRobot::chassisInfoCallback, this, std::placeholders::_1));

    timer_ = create_wall_timer(10ms, std::bind(&ControlRobot::run, this));

    RCLCPP_INFO(get_logger(), "Begin Control Robot.");
    RCLCPP_INFO(get_logger(), "Publishing odometry on %s", odom_topic_.c_str());
  }

private:
  static constexpr uint8_t GEAR_NONE = 0;
  static constexpr uint8_t GEAR_PARK = 1;
  static constexpr uint8_t GEAR_NEUTRAL = 2;
  static constexpr uint8_t GEAR_ACKERMANN = 6;
  static constexpr uint8_t GEAR_SLIDE = 7;
  static constexpr double PI = 3.14159265358979323846;
  static constexpr double MAX_OBLIQUE_RADIAN = 35.0 * PI / 180.0;
  static constexpr double MAX_ROTATE_VEL_DEG = 10.0;

  struct WheelState
  {
    double position{0.0};
    double velocity{0.0};
    double sign{1.0};
    std::optional<int64_t> last_pulse;
    std::optional<rclcpp::Time> last_update_time;
  };

  static double radToDeg(const double rad)
  {
    return rad * 180.0 / PI;
  }

  static double degToRad(const double deg)
  {
    return deg * PI / 180.0;
  }

  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    const double vx = msg->linear.x;
    const double vy = msg->linear.y;
    const double vtheta = msg->angular.z;

    if (vy != 0.0) {
      if ((vx == 0.0 || (std::abs(vx) < 0.05 && std::abs(vy) < 0.12)) && vtheta != 0.0) {
        // rotate
        gear_ = GEAR_ACKERMANN;
        linear_ = 0.0;
        angular_ = radToDeg(vtheta);
        slip_angle_ = 0.0;
        is_rotating_ = true;
        is_parked_ = false;
      } else if (vx != 0.0) {
        const double alpha = std::atan2(vy, vx) + vtheta;
        RCLCPP_DEBUG(get_logger(), "alpha=%.6f", alpha);

        gear_ = GEAR_SLIDE;
        linear_ = std::hypot(vx, vy) * (vx < 0.0 ? -1.0 : 1.0);
        angular_ = 0.0;
        is_parked_ = false;

        if (-MAX_OBLIQUE_RADIAN <= alpha && alpha <= MAX_OBLIQUE_RADIAN) {
          slip_angle_ = radToDeg(alpha);
        } else if (
          (-PI - MAX_OBLIQUE_RADIAN) <= alpha &&
          alpha <= (-PI + MAX_OBLIQUE_RADIAN))
        {
          slip_angle_ = radToDeg(alpha + PI);
        } else if (
          (PI - MAX_OBLIQUE_RADIAN) <= alpha &&
          alpha <= (PI + MAX_OBLIQUE_RADIAN))
        {
          slip_angle_ = radToDeg(alpha - PI);
        } else {
          gear_ = GEAR_ACKERMANN;
          linear_ = 0.0;
          slip_angle_ = 0.0;
          is_rotating_ = true;
          is_parked_ = false;

          if (vx * vy >= 0.0) {
            angular_ = MAX_ROTATE_VEL_DEG;
          } else {
            angular_ = -MAX_ROTATE_VEL_DEG;
          }
        }
      } else if (vy == 0.0 && vtheta != 0.0) {
        gear_ = GEAR_ACKERMANN;
        linear_ = 0.0;
        angular_ = radToDeg(vtheta);
        slip_angle_ = 0.0;
        is_rotating_ = true;
        is_parked_ = false;
      } else {
        setParkCommand();
      }
    } else {
      if (std::abs(vx) < 0.05 && vtheta != 0.0) {
        // rotate
        gear_ = GEAR_ACKERMANN;
        linear_ = 0.0;
        angular_ = vtheta > 0.0 ?
          std::min(radToDeg(vtheta), MAX_ROTATE_VEL_DEG) :
          std::max(radToDeg(vtheta), -MAX_ROTATE_VEL_DEG);
        slip_angle_ = 0.0;
        is_rotating_ = true;
        is_parked_ = false;
      } else if (vx != 0.0) {
        // ackermann
        gear_ = GEAR_ACKERMANN;
        linear_ = vx;
        angular_ = radToDeg(vtheta);
        slip_angle_ = 0.0;
        is_rotating_ = false;
        is_parked_ = false;
      } else {
        setParkCommand();
      }
    }
  }

  void setParkCommand()
  {
    gear_ = GEAR_PARK;
    linear_ = 0.0;
    angular_ = 0.0;
    slip_angle_ = 0.0;
  }

  void chassisInfoCallback(const yhs_can_interfaces::msg::ChassisInfoFb::SharedPtr msg)
  {

    const rclcpp::Time current_time = now();

    updateWheel(
      wheel_lf_,
      msg->lf_wheel_fb.lf_wheel_fb_velocity,
      msg->lf_wheel_fb.lf_wheel_fb_pulse,
      current_time);

    updateWheel(
      wheel_rf_,
      msg->rf_wheel_fb.rf_wheel_fb_velocity,
      msg->rf_wheel_fb.rf_wheel_fb_pulse,
      current_time);

    updateWheel(
      wheel_lr_,
      msg->lr_wheel_fb.lr_wheel_fb_velocity,
      msg->lr_wheel_fb.lr_wheel_fb_pulse,
      current_time);

    updateWheel(
      wheel_rr_,
      msg->rr_wheel_fb.rr_wheel_fb_velocity,
      msg->rr_wheel_fb.rr_wheel_fb_pulse,
      current_time);

    steer_lf_ = degToRad(msg->front_angle_fb.front_angle_fb_l);
    steer_rf_ = degToRad(msg->front_angle_fb.front_angle_fb_r);
    steer_lr_ = degToRad(msg->rear_angle_fb.rear_angle_fb_l);
    steer_rr_ = degToRad(msg->rear_angle_fb.rear_angle_fb_r);

    const rclcpp::Time feedback_stamp(msg->header.stamp);
    const int64_t feedback_stamp_ns = feedback_stamp.nanoseconds();

    bool is_new_ctrl_feedback = false;
    if (feedback_stamp_ns > 0) {
      if (!last_ctrl_feedback_stamp_ns_.has_value() ||
        feedback_stamp_ns != last_ctrl_feedback_stamp_ns_.value())
      {
        last_ctrl_feedback_stamp_ns_ = feedback_stamp_ns;
        is_new_ctrl_feedback = true;
      }
    } else {
      is_new_ctrl_feedback = true;
    }

    if (is_new_ctrl_feedback) {
      ctrlFbCallback(msg->ctrl_fb, current_time);
    }
  }

  void updateWheel(
    WheelState & wheel,
    const double velocity_mps,
    const int64_t pulse,
    const rclcpp::Time & current_time)
  {
    const double omega = wheel_radius_ > 1e-6 ?
      (velocity_mps / wheel_radius_) * wheel.sign : 0.0;

    wheel.velocity = omega;

    if (!wheel.last_pulse.has_value()) {
      wheel.last_pulse = pulse;
      wheel.last_update_time = current_time;
      return;
    }

    if (wheel.last_pulse.value() == pulse) {
      return;
    }

    if (!wheel.last_update_time.has_value()) {
      wheel.last_pulse = pulse;
      wheel.last_update_time = current_time;
      return;
    }

    const double dt = (current_time - wheel.last_update_time.value()).seconds();

    wheel.last_pulse = pulse;
    wheel.last_update_time = current_time;

    if (dt <= 0.0 || dt > 0.5) {
      return;
    }

    wheel.position += omega * dt;
  }

  void ctrlFbCallback(
    const yhs_can_interfaces::msg::CtrlFb & msg,
    const rclcpp::Time & current_time)
  {
    const double linear_vel = msg.ctrl_fb_x_linear;
    const double angular_vel = degToRad(msg.ctrl_fb_z_angular);
    const double slip_angle = degToRad(msg.ctrl_fb_y_linear);

    const double dt = (current_time - last_odom_time_).seconds();

    if (dt > 0.0 && dt <= 0.5) {
      if (msg.ctrl_fb_gear == GEAR_ACKERMANN) {
        x_ += linear_vel * std::cos(theta_) * dt;
        y_ += linear_vel * std::sin(theta_) * dt;
        theta_ += angular_vel * dt;
      } else if (msg.ctrl_fb_gear == GEAR_SLIDE) {
        x_ += linear_vel * std::cos(slip_angle + theta_) * dt;
        y_ += linear_vel * std::sin(slip_angle + theta_) * dt;
      }
    }

    publishOdom(current_time, linear_vel, angular_vel);
    publishJointStates(current_time);

    last_odom_time_ = current_time;
  }

  void publishOdom(
    const rclcpp::Time & current_time,
    const double linear_vel,
    const double angular_vel)
  {
    nav_msgs::msg::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = odom_frame_id_;
    odom.child_frame_id = base_frame_id_;

    odom.pose.pose.position.x = x_;
    odom.pose.pose.position.y = y_;
    odom.pose.pose.position.z = 0.0;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, theta_);
    odom.pose.pose.orientation.x = q.x();
    odom.pose.pose.orientation.y = q.y();
    odom.pose.pose.orientation.z = q.z();
    odom.pose.pose.orientation.w = q.w();

    odom.twist.twist.linear.x = linear_vel;
    odom.twist.twist.linear.y = 0.0;
    odom.twist.twist.linear.z = 0.0;
    odom.twist.twist.angular.x = 0.0;
    odom.twist.twist.angular.y = 0.0;
    odom.twist.twist.angular.z = angular_vel;

    odom_pub_->publish(odom);
  }

  void publishJointStates(const rclcpp::Time & current_time)
  {
    sensor_msgs::msg::JointState joint_state;
    joint_state.header.stamp = current_time;

    joint_state.name = {
      "left_steering_hinge_joint",
      "right_steering_hinge_joint",
      "rear_left_steering_hinge_joint",
      "rear_right_steering_hinge_joint",
      "left_front_wheel_joint",
      "right_front_wheel_joint",
      "left_rear_wheel_joint",
      "right_rear_wheel_joint"
    };

    joint_state.position = {
      steer_lf_, steer_rf_, steer_lr_, steer_rr_,
      wheel_lf_.position, wheel_rf_.position, wheel_lr_.position, wheel_rr_.position
    };

    joint_state.velocity = {
      0.0, 0.0, 0.0, 0.0,
      wheel_lf_.velocity, wheel_rf_.velocity, wheel_lr_.velocity, wheel_rr_.velocity
    };

    joint_state_pub_->publish(joint_state);
  }

  void publishCurrentCommand()
  {
    robot_cmd_.ctrl_cmd_gear = gear_;
    robot_cmd_.ctrl_cmd_x_linear = linear_;
    robot_cmd_.ctrl_cmd_z_angular = angular_;
    robot_cmd_.ctrl_cmd_y_linear = slip_angle_;

    count_ = 0;
    ctrl_cmd_pub_->publish(robot_cmd_);
  }

  void run()
  {
    if (gear_ == GEAR_PARK && !is_parked_) {
      if (count_ < 100) {
        robot_cmd_.ctrl_cmd_x_linear = linear_;
        robot_cmd_.ctrl_cmd_z_angular = angular_;
        robot_cmd_.ctrl_cmd_y_linear = slip_angle_;

        ++count_;
        ctrl_cmd_pub_->publish(robot_cmd_);
        return;
      }

      is_parked_ = true;
    } else if (gear_ == GEAR_ACKERMANN) {
      if (is_rotating_) {
        if (!is_rotated_ && count_ < 100) {
          robot_cmd_.ctrl_cmd_gear = gear_;
          robot_cmd_.ctrl_cmd_x_linear = 0.0;
          robot_cmd_.ctrl_cmd_z_angular = angular_;
          robot_cmd_.ctrl_cmd_y_linear = 0.0;

          ++count_;
          change_gear_ = true;
          RCLCPP_DEBUG(get_logger(), "ACKERMANN rotate count=%d", count_);
          ctrl_cmd_pub_->publish(robot_cmd_);
          return;
        }

        change_gear_ = false;
        is_rotated_ = true;
      } else {
        if (change_gear_) {
          change_gear_ = false;
          is_rotated_ = true;
          count_ = 0;
        }

        if (is_rotated_) {
          if (count_ < 100) {
            robot_cmd_.ctrl_cmd_gear = gear_;
            robot_cmd_.ctrl_cmd_x_linear = 0.0;
            robot_cmd_.ctrl_cmd_z_angular = 0.0;
            robot_cmd_.ctrl_cmd_y_linear = 0.0;

            ++count_;
            RCLCPP_DEBUG(get_logger(), "ACKERMANN settle count=%d", count_);
            ctrl_cmd_pub_->publish(robot_cmd_);
            return;
          }

          is_rotated_ = false;
        }
      }
    } else if (gear_ == GEAR_SLIDE) {
      if (change_gear_) {
        change_gear_ = false;
        is_rotated_ = true;
        count_ = 0;
      }

      if (is_rotated_) {
        if (count_ < 100) {
          robot_cmd_.ctrl_cmd_gear = gear_;
          robot_cmd_.ctrl_cmd_x_linear = 0.0;
          robot_cmd_.ctrl_cmd_z_angular = angular_;
          robot_cmd_.ctrl_cmd_y_linear = 0.0;

          ++count_;
          is_rotated_ = true;
          RCLCPP_DEBUG(get_logger(), "SLIDE settle count=%d", count_);
          ctrl_cmd_pub_->publish(robot_cmd_);
          return;
        }

        is_rotated_ = false;
      }

      if (is_slip_reverse_) {
        if (count_ < 100) {
          robot_cmd_.ctrl_cmd_gear = gear_;
          robot_cmd_.ctrl_cmd_x_linear = 0.0;
          robot_cmd_.ctrl_cmd_z_angular = 0.0;
          robot_cmd_.ctrl_cmd_y_linear = slip_angle_;

          ++count_;
          RCLCPP_DEBUG(get_logger(), "SLIDE reverse count=%d", count_);
          ctrl_cmd_pub_->publish(robot_cmd_);
          return;
        }

        is_slip_reverse_ = false;
      }
    }

    publishCurrentCommand();
  }

  rclcpp::Publisher<yhs_can_interfaces::msg::CtrlCmd>::SharedPtr ctrl_cmd_pub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<yhs_can_interfaces::msg::ChassisInfoFb>::SharedPtr chassis_info_sub_;

  rclcpp::TimerBase::SharedPtr timer_;

  yhs_can_interfaces::msg::CtrlCmd robot_cmd_;

  int count_{0};
  uint8_t gear_{GEAR_NONE};
  bool is_rotating_{false};
  bool change_gear_{false};
  bool is_rotated_{false};
  bool is_parked_{false};
  bool is_slip_reverse_{false};

  double linear_{0.0};
  double angular_{0.0};
  double slip_angle_{0.0};

  double wheel_radius_{0.07};
  WheelState wheel_lf_;
  WheelState wheel_rf_;
  WheelState wheel_lr_;
  WheelState wheel_rr_;

  double steer_lf_{0.0};
  double steer_rf_{0.0};
  double steer_lr_{0.0};
  double steer_rr_{0.0};

  double x_{0.0};
  double y_{0.0};
  double theta_{0.0};
  rclcpp::Time last_odom_time_;
  std::optional<int64_t> last_ctrl_feedback_stamp_ns_;

  std::string odom_topic_;
  std::string odom_frame_id_;
  std::string base_frame_id_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ControlRobot>());
  rclcpp::shutdown();
  return 0;
}
