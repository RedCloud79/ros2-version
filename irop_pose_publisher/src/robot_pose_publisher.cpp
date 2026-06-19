/*!
 * \file robot_pose_publisher.cpp
 * \brief Publishes the robot position relative to the map frame.
 *
 * ROS2 Humble conversion of robot_pose_publisher.
 *
 * The node reads the TF transform between map_frame and base_frame,
 * then publishes the robot pose on /robot_pose.
 */

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.hpp"
#include "tf2/time.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"


class RobotPosePublisher : public rclcpp::Node
{
public:
  RobotPosePublisher()
  : Node("robot_pose_publisher")
  {
    /*
     * ROS1 기본값:
     *   map_frame         = "/map"
     *   base_frame        = "/base_link"
     *   publish_frequency = 10 Hz
     *   is_stamped        = false
     *
     * ROS2에서는 frame_id 앞의 "/"를 일반적으로 사용하지 않으므로
     * "map", "base_link"로 설정한다.
     */
    map_frame_ =
      this->declare_parameter<std::string>(
      "map_frame",
      "map");

    base_frame_ =
      this->declare_parameter<std::string>(
      "base_frame",
      "base_link");

    publish_frequency_ =
      this->declare_parameter<double>(
      "publish_frequency",
      10.0);

    is_stamped_ =
      this->declare_parameter<bool>(
      "is_stamped",
      false);

    if (publish_frequency_ <= 0.0) {
      RCLCPP_WARN(
        this->get_logger(),
        "publish_frequency must be greater than zero. "
        "Using the default value: 10.0 Hz");

      publish_frequency_ = 10.0;
    }

    /*
     * is_stamped 값에 따라 기존 ROS1과 동일하게 메시지 타입을 선택한다.
     *
     * false:
     *   /robot_pose
     *   geometry_msgs/msg/Pose
     *
     * true:
     *   /robot_pose
     *   geometry_msgs/msg/PoseStamped
     */
    if (is_stamped_) {
      pose_stamped_pub_ =
        this->create_publisher<geometry_msgs::msg::PoseStamped>(
        "robot_pose",
        1);

      RCLCPP_INFO(
        this->get_logger(),
        "Publishing geometry_msgs/msg/PoseStamped on /robot_pose");
    } else {
      pose_pub_ =
        this->create_publisher<geometry_msgs::msg::Pose>(
        "robot_pose",
        1);

      RCLCPP_INFO(
        this->get_logger(),
        "Publishing geometry_msgs/msg/Pose on /robot_pose");
    }

    tf_buffer_ =
      std::make_unique<tf2_ros::Buffer>(
      this->get_clock());

    tf_listener_ =
      std::make_shared<tf2_ros::TransformListener>(
      *tf_buffer_);

    const auto period =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double>(
        1.0 / publish_frequency_));

    timer_ =
      this->create_wall_timer(
      period,
      std::bind(
        &RobotPosePublisher::publishRobotPose,
        this));

    RCLCPP_INFO(
      this->get_logger(),
      "irop_pose_publisher started: %s -> %s, %.2f Hz",
      map_frame_.c_str(),
      base_frame_.c_str(),
      publish_frequency_);
  }

private:
  std::string map_frame_;
  std::string base_frame_;

  double publish_frequency_;
  bool is_stamped_;

  std::unique_ptr<tf2_ros::Buffer>
    tf_buffer_;

  std::shared_ptr<tf2_ros::TransformListener>
    tf_listener_;

  rclcpp::Publisher<geometry_msgs::msg::Pose>::SharedPtr
    pose_pub_;

  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr
    pose_stamped_pub_;

  rclcpp::TimerBase::SharedPtr
    timer_;

  void publishRobotPose()
  {
    try {
      /*
       * 가장 최근의 map -> base_link TF를 조회한다.
       *
       * ROS1:
       *   listener.lookupTransform(
       *     map_frame,
       *     base_frame,
       *     ros::Time(0),
       *     transform);
       *
       * ROS2:
       *   tf_buffer_->lookupTransform(
       *     map_frame_,
       *     base_frame_,
       *     tf2::TimePointZero);
       */
      const auto transform =
        tf_buffer_->lookupTransform(
        map_frame_,
        base_frame_,
        tf2::TimePointZero);

      geometry_msgs::msg::PoseStamped
        pose_stamped;

      pose_stamped.header.frame_id =
        map_frame_;

      pose_stamped.header.stamp =
        this->now();

      pose_stamped.pose.position.x =
        transform.transform.translation.x;

      pose_stamped.pose.position.y =
        transform.transform.translation.y;

      pose_stamped.pose.position.z =
        transform.transform.translation.z;

      pose_stamped.pose.orientation.x =
        transform.transform.rotation.x;

      pose_stamped.pose.orientation.y =
        transform.transform.rotation.y;

      pose_stamped.pose.orientation.z =
        transform.transform.rotation.z;

      pose_stamped.pose.orientation.w =
        transform.transform.rotation.w;

      if (is_stamped_) {
        pose_stamped_pub_->publish(
          pose_stamped);
      } else {
        pose_pub_->publish(
          pose_stamped.pose);
      }
    } catch (const tf2::TransformException & exception) {
      /*
       * 기존 ROS1 코드도 TF 조회 실패 시 종료하지 않고
       * 다음 주기에 다시 시도한다.
       *
       * 원인 확인이 가능하도록 2초 간격으로만 경고를 출력한다.
       */
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        2000,
        "Could not transform %s -> %s: %s",
        map_frame_.c_str(),
        base_frame_.c_str(),
        exception.what());
    }
  }
};


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
    std::make_shared<RobotPosePublisher>());

  rclcpp::shutdown();

  return 0;
}