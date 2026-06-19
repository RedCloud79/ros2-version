#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "sensor_msgs/msg/range.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "std_msgs/msg/int8.hpp"


class SafetyManager : public rclcpp::Node
{
public:
  SafetyManager()
  : Node("safety_manager"),
    cmd_mux_(0),
    temp_sensor_(0),
    scan_received_(false)
  {
    using std::placeholders::_1;

    array_sensor_.fill(0.0);

    scan_sub_ =
      this->create_subscription<sensor_msgs::msg::LaserScan>(
      "/scan",
      rclcpp::SensorDataQoS(),
      std::bind(&SafetyManager::scanCallback, this, _1));

    cmd_vel_move_sub_ =
      this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel_move",
      10,
      std::bind(&SafetyManager::cmdVelMoveCallback, this, _1));

    cmd_vel_dock_sub_ =
      this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel_dock",
      10,
      std::bind(&SafetyManager::cmdVelDockCallback, this, _1));

    cmd_vel_begin_sub_ =
      this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel_begin",
      10,
      std::bind(&SafetyManager::cmdVelBeginCallback, this, _1));

    cmd_mux_sub_ =
      this->create_subscription<std_msgs::msg::Int8>(
      "cmd_mux_choose",
      10,
      std::bind(&SafetyManager::cmdMuxCallback, this, _1));

    /*
     * 기존 ROS1 토픽 이름에 utralsonic 오타가 포함되어 있음.
     * 기존 발행 노드와 연결을 유지하기 위해 이름을 바꾸지 않음.
     */
    ultrasonic_sub_ =
      this->create_subscription<std_msgs::msg::Float64MultiArray>(
      "utralsonic_filter",
      10,
      std::bind(&SafetyManager::ultrasonicCallback, this, _1));

    cmd_vel_pub_ =
      this->create_publisher<geometry_msgs::msg::Twist>(
      "cmd_vel",
      10);

    range_sensor_02_pub_ =
      this->create_publisher<sensor_msgs::msg::Range>(
      "range_sensor_02",
      10);

    range_sensor_01_pub_ =
      this->create_publisher<sensor_msgs::msg::Range>(
      "range_sensor_01",
      10);

    /*
     * 기존 ROS1 루프: rospy.Rate(25)
     * 1000 ms / 25 Hz = 40 ms
     */
    timer_ =
      this->create_wall_timer(
      std::chrono::milliseconds(40),
      std::bind(&SafetyManager::controlLoop, this));

    RCLCPP_INFO(
      this->get_logger(),
      "irop_safety_manager started");
  }

private:
  static constexpr double kPi = 3.14159265358979323846;
  static constexpr double kApproxPi = 3.14;
  static constexpr double kHalfPi = 1.57;

  static constexpr double kLengthRobot = 0.425;
  static constexpr double kWidthRobot = 0.35;

  static constexpr double kNormalDistanceLimit = 0.475;
  static constexpr double kDockDistanceLimit = 0.33;
  static constexpr double kRotateSideDistance = 0.372;

  static constexpr double kSideAngleFactor = 0.7225663;

  const double beta_ = std::atan(kWidthRobot / kLengthRobot);

  geometry_msgs::msg::Twist cmd_vel_;
  geometry_msgs::msg::Twist cmd_vel_move_;
  geometry_msgs::msg::Twist cmd_vel_dock_;
  geometry_msgs::msg::Twist cmd_vel_begin_;

  sensor_msgs::msg::LaserScan lidar_scan_;

  std::array<double, 8> array_sensor_;

  int8_t cmd_mux_;
  int temp_sensor_;
  bool scan_received_;

  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr
    scan_sub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr
    cmd_vel_move_sub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr
    cmd_vel_dock_sub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr
    cmd_vel_begin_sub_;

  rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr
    cmd_mux_sub_;

  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr
    ultrasonic_sub_;

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr
    cmd_vel_pub_;

  rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr
    range_sensor_02_pub_;

  rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr
    range_sensor_01_pub_;

  rclcpp::TimerBase::SharedPtr timer_;

  void scanCallback(
    const sensor_msgs::msg::LaserScan::SharedPtr msg)
  {
    lidar_scan_ = *msg;
    scan_received_ = true;
  }

  void cmdVelMoveCallback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    cmd_vel_move_ = *msg;
  }

  void cmdVelDockCallback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    cmd_vel_dock_ = *msg;
  }

  void cmdVelBeginCallback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    cmd_vel_begin_ = *msg;
  }

  void cmdMuxCallback(
    const std_msgs::msg::Int8::SharedPtr msg)
  {
    cmd_mux_ = msg->data;
  }

  void ultrasonicCallback(
    const std_msgs::msg::Float64MultiArray::SharedPtr msg)
  {
    if (msg->data.size() < array_sensor_.size()) {
      RCLCPP_WARN(
        this->get_logger(),
        "utralsonic_filter requires 8 values, but received %zu",
        msg->data.size());

      return;
    }

    for (std::size_t index = 0;
      index < array_sensor_.size();
      ++index)
    {
      array_sensor_[index] = msg->data[index];
    }
  }

  bool scanReady() const
  {
    return scan_received_ &&
           !lidar_scan_.ranges.empty() &&
           std::abs(lidar_scan_.angle_increment) > 1.0e-12;
  }

  int angleToIndex(const double angle) const
  {
    return static_cast<int>(
      (angle - static_cast<double>(lidar_scan_.angle_min)) /
      static_cast<double>(lidar_scan_.angle_increment));
  }

  void appendRanges(
    std::vector<double> & values,
    const int begin,
    const int end) const
  {
    if (begin >= end) {
      return;
    }

    for (int index = begin; index < end; ++index) {
      if (index < 0 ||
          index >= static_cast<int>(lidar_scan_.ranges.size()))
      {
        continue;
      }

      const double value =
        lidar_scan_.ranges[static_cast<std::size_t>(index)];

      /*
       * inf는 장애물이 없는 정상 LaserScan 값일 수 있으므로 유지.
       * NaN만 제외.
       */
      if (!std::isnan(value)) {
        values.push_back(value);
      }
    }
  }

  bool minLessThan(
    const std::vector<double> & values,
    const double threshold) const
  {
    if (values.empty()) {
      return false;
    }

    return *std::min_element(
      values.begin(),
      values.end()) < threshold;
  }

  double noLeft() const
  {
    if (cmd_vel_.angular.z > 0.0) {
      return 0.0;
    }

    return cmd_vel_.angular.z;
  }

  double noRight() const
  {
    if (cmd_vel_.angular.z < 0.0) {
      return 0.0;
    }

    return cmd_vel_.angular.z;
  }

  bool checkFront(
    const double limit,
    const double alpha) const
  {
    std::vector<double> values;

    const double x =
      std::sqrt(
      kWidthRobot * kWidthRobot +
      kLengthRobot * kLengthRobot) *
      std::sin(std::abs(alpha) + beta_);

    const double angle_width =
      std::abs(std::atan2(x, limit));

    appendRanges(
      values,
      angleToIndex(-alpha - angle_width),
      angleToIndex(-alpha + angle_width));

    return minLessThan(values, limit);
  }

  bool checkBack(
    const double limit,
    double alpha) const
  {
    std::vector<double> values;

    if ((alpha <= -(-kHalfPi + kPi)) &&
        (alpha >= -kPi))
    {
      alpha += kPi;
    } else if (
      (alpha >= (-kHalfPi + kPi)) &&
      (alpha <= kPi))
    {
      alpha -= kPi;
    }

    const double x =
      std::sqrt(
      kWidthRobot * kWidthRobot +
      kLengthRobot * kLengthRobot) *
      std::sin(std::abs(alpha) + beta_);

    const double angle_width =
      std::abs(std::atan2(x, limit));

    appendRanges(
      values,
      angleToIndex(kApproxPi - alpha - angle_width),
      angleToIndex(kApproxPi - alpha + angle_width));

    return minLessThan(values, limit);
  }

  bool checkMinFront(
    const double limit,
    const double alpha) const
  {
    /*
     * 원본 코드의 초기값 [99] 유지.
     */
    std::vector<double> a{99.0};
    std::vector<double> b{99.0};

    const double x =
      std::sqrt(
      kWidthRobot * kWidthRobot +
      kLengthRobot * kLengthRobot) *
      std::sin(std::abs(alpha) + beta_);

    const double angle_width =
      std::abs(std::atan2(x, limit));

    appendRanges(
      a,
      angleToIndex(-alpha - angle_width),
      angleToIndex(-alpha + angle_width));

    if (alpha >= 0.0) {
      if (alpha != 0.0) {
        appendRanges(
          b,
          angleToIndex(-alpha + angle_width),
          angleToIndex(beta_));

        appendRanges(
          b,
          angleToIndex(-kApproxPi + beta_),
          angleToIndex(-alpha - angle_width));
      }
    } else {
      appendRanges(
        b,
        angleToIndex(-beta_),
        angleToIndex(-alpha - angle_width));

      appendRanges(
        b,
        angleToIndex(-alpha + angle_width),
        angleToIndex(kApproxPi - beta_));
    }

    return minLessThan(a, limit) ||
           minLessThan(b, kWidthRobot + 0.025);
  }

  bool checkMinBack(
    const double limit,
    double alpha) const
  {
    /*
     * 원본 코드의 초기값 [99] 유지.
     */
    std::vector<double> a{99.0};
    std::vector<double> b{99.0};

    if ((alpha <= -(-kHalfPi + kPi)) &&
        (alpha >= -kPi))
    {
      alpha += kPi;
    } else if (
      (alpha >= (-kHalfPi + kPi)) &&
      (alpha <= kPi))
    {
      alpha -= kPi;
    }

    const double x =
      std::sqrt(
      kWidthRobot * kWidthRobot +
      kLengthRobot * kLengthRobot) *
      std::sin(std::abs(alpha) + beta_);

    const double angle_width =
      std::abs(std::atan2(x, limit));

    appendRanges(
      a,
      angleToIndex(kApproxPi - alpha - angle_width),
      angleToIndex(kApproxPi - alpha + angle_width));

    if (alpha >= 0.0) {
      if (alpha != 0.0) {
        appendRanges(
          b,
          angleToIndex(beta_),
          angleToIndex(kApproxPi - alpha - angle_width));

        appendRanges(
          b,
          angleToIndex(kApproxPi - alpha + angle_width),
          angleToIndex(kApproxPi + beta_));
      }
    } else {
      appendRanges(
        b,
        angleToIndex(kApproxPi - beta_),
        angleToIndex(kApproxPi - alpha - angle_width));

      appendRanges(
        b,
        angleToIndex(kApproxPi - alpha + angle_width),
        angleToIndex(
          static_cast<double>(lidar_scan_.angle_max) -
          beta_));
    }

    return minLessThan(a, limit) ||
           minLessThan(b, kWidthRobot + 0.025);
  }

  bool checkLeft(const double limit) const
  {
    std::vector<double> values;

    const double angle =
      std::asin(kSideAngleFactor);

    appendRanges(
      values,
      angleToIndex(kHalfPi - angle),
      angleToIndex(kHalfPi));

    appendRanges(
      values,
      angleToIndex(
        static_cast<double>(lidar_scan_.angle_max) -
        kHalfPi -
        angle),
      angleToIndex(
        static_cast<double>(lidar_scan_.angle_max) -
        kHalfPi));

    return minLessThan(values, limit);
  }

  bool checkRight(const double limit) const
  {
    std::vector<double> values;

    const double angle =
      std::asin(kSideAngleFactor);

    appendRanges(
      values,
      angleToIndex(kHalfPi),
      angleToIndex(kHalfPi + angle));

    appendRanges(
      values,
      angleToIndex(
        static_cast<double>(lidar_scan_.angle_max) -
        kHalfPi),
      angleToIndex(
        static_cast<double>(lidar_scan_.angle_max) -
        kHalfPi +
        angle));

    return minLessThan(values, limit);
  }

  geometry_msgs::msg::Twist muxCheck() const
  {
    if (cmd_mux_ == 0) {
      return cmd_vel_move_;
    }

    if (cmd_mux_ == 1) {
      return cmd_vel_begin_;
    }

    if (cmd_mux_ == 2) {
      return cmd_vel_dock_;
    }

    return geometry_msgs::msg::Twist();
  }

  void publishUltrasonicRange()
  {
    sensor_msgs::msg::Range range_sensor_02;

    range_sensor_02.header.stamp =
      this->now();

    range_sensor_02.header.frame_id =
      "ultra_sensor_link_02";

    range_sensor_02.radiation_type = 0;
    range_sensor_02.min_range = 0.01;
    range_sensor_02.max_range = 5.0;
    range_sensor_02.field_of_view = kApproxPi / 6.0;
    range_sensor_02.range = array_sensor_[2] / 100.0;

    sensor_msgs::msg::Range range_sensor_01;

    range_sensor_01.header.stamp =
      this->now();

    range_sensor_01.header.frame_id =
      "ultra_sensor_link_01";

    range_sensor_01.radiation_type = 0;
    range_sensor_01.min_range = 0.01;
    range_sensor_01.max_range = 5.0;
    range_sensor_01.field_of_view = kApproxPi / 6.0;
    range_sensor_01.range = array_sensor_[5] / 100.0;

    range_sensor_02_pub_->publish(range_sensor_02);
    range_sensor_01_pub_->publish(range_sensor_01);
  }

  bool checkUltrasonicSensors(
    const double vx,
    const double vy,
    const double vw)
  {
    static constexpr std::array<double, 8>
      check_distance{
      30.0,
      36.0,
      30.0,
      30.0,
      20.0,
      31.0,
      31.0,
      36.0
    };

    publishUltrasonicRange();

    std::vector<double> array_check;
    std::vector<double> data_check;

    const auto addSensor =
      [&](const std::size_t index)
      {
        array_check.push_back(array_sensor_[index]);
        data_check.push_back(check_distance[index]);
      };

    if (vx > 0.0) {
      if (vy > 0.0) {
        addSensor(0);
        addSensor(1);
        addSensor(2);
      } else if (vy < 0.0) {
        addSensor(1);
        addSensor(2);
        addSensor(3);
      } else {
        addSensor(1);
        addSensor(2);
      }
    } else if (vx < 0.0) {
      if (vy > 0.0) {
        addSensor(5);
        addSensor(6);
        addSensor(7);
      } else if (vy < 0.0) {
        addSensor(4);
        addSensor(5);
        addSensor(6);
      } else {
        addSensor(5);
        addSensor(6);
      }
    } else if (vw > 0.0) {
      addSensor(0);
      addSensor(4);
    } else if (vw < 0.0) {
      addSensor(3);
      addSensor(7);
    }

    const int previous_temp_sensor =
      temp_sensor_;

    for (std::size_t index = 0;
      index < array_check.size();
      ++index)
    {
      if (array_check[index] < data_check[index]) {
        ++temp_sensor_;
        break;
      }
    }

    if (temp_sensor_ == previous_temp_sensor) {
      temp_sensor_ = 0;
    }

    return temp_sensor_ >= 5;
  }

  geometry_msgs::msg::Twist safetyControl(
    const geometry_msgs::msg::Twist & cmd_vel_in,
    const double distance_limit)
  {
    geometry_msgs::msg::Twist cmd_vel_out =
      cmd_vel_in;

    const double vx =
      cmd_vel_in.linear.x;

    const double vy =
      cmd_vel_in.linear.y;

    const double vw =
      cmd_vel_in.angular.z;

    /*
     * 원본 ROS1 코드와 동일하게 초음파 감지는 로그만 출력.
     * 속도 정지에는 직접 반영하지 않음.
     */
    if (checkUltrasonicSensors(vx, vy, vw)) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        1000,
        "Ultrasonic sensor obstacle detected");
    }

    const bool motion_requested =
      std::abs(vx) > 0.0 ||
      std::abs(vy) > 0.0 ||
      std::abs(vw) > 0.0;

    /*
     * ROS1 원본은 /scan 수신 전에 인덱스 오류가 발생할 수 있음.
     * ROS2 변환본은 초기 구동 중 안전하게 0 속도 발행.
     */
    if (motion_requested && !scanReady()) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        2000,
        "LaserScan is not ready. Publishing zero velocity.");

      return geometry_msgs::msg::Twist();
    }

    if (cmd_vel_in.linear.x > 0.0) {
      const double alpha =
        std::atan2(
        cmd_vel_out.linear.y,
        cmd_vel_out.linear.x);

      if (checkMinFront(distance_limit, alpha)) {
        cmd_vel_out.linear.x = 0.0;
        cmd_vel_out.linear.y = 0.0;
      } else if (
        checkFront(distance_limit + 0.1, alpha))
      {
        cmd_vel_out.linear.x = 0.2 * vx;
        cmd_vel_out.linear.y = 0.2 * vy;
      } else if (
        checkFront(distance_limit + 0.2, alpha))
      {
        cmd_vel_out.linear.x = 0.4 * vx;
        cmd_vel_out.linear.y = 0.4 * vy;
      } else if (
        checkFront(distance_limit + 0.3, alpha))
      {
        cmd_vel_out.linear.x = 0.6 * vx;
        cmd_vel_out.linear.y = 0.6 * vy;
      } else if (
        checkFront(distance_limit + 0.4, alpha))
      {
        cmd_vel_out.linear.x = 0.8 * vx;
        cmd_vel_out.linear.y = 0.8 * vy;
      }
    }

    if (cmd_vel_in.linear.x < 0.0) {
      const double alpha =
        std::atan2(
        cmd_vel_out.linear.y,
        cmd_vel_out.linear.x);

      if (checkMinBack(distance_limit, alpha)) {
        cmd_vel_out.linear.x = 0.0;
        cmd_vel_out.linear.y = 0.0;
      } else if (
        checkBack(distance_limit + 0.1, alpha))
      {
        cmd_vel_out.linear.x = 0.2 * vx;
        cmd_vel_out.linear.y = 0.2 * vy;
      } else if (
        checkBack(distance_limit + 0.2, alpha))
      {
        cmd_vel_out.linear.x = 0.4 * vx;
        cmd_vel_out.linear.y = 0.4 * vy;
      } else if (
        checkBack(distance_limit + 0.3, alpha))
      {
        cmd_vel_out.linear.x = 0.6 * vx;
        cmd_vel_out.linear.y = 0.6 * vy;
      } else if (
        checkBack(distance_limit + 0.4, alpha))
      {
        cmd_vel_out.linear.x = 0.8 * vx;
        cmd_vel_out.linear.y = 0.8 * vy;
      }
    }

    if (cmd_vel_in.angular.z > 0.0 &&
        checkLeft(kRotateSideDistance))
    {
      cmd_vel_out.angular.z =
        noLeft();

      RCLCPP_INFO(
        this->get_logger(),
        "no left");
    }

    if (cmd_vel_in.angular.z < 0.0 &&
        checkRight(kRotateSideDistance))
    {
      cmd_vel_out.angular.z =
        noRight();

      RCLCPP_INFO(
        this->get_logger(),
        "no right");
    }

    return cmd_vel_out;
  }

  void controlLoop()
  {
    cmd_vel_ = muxCheck();

    if (cmd_mux_ < 0 ||
        cmd_mux_ > 2)
    {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        2000,
        "Unknown cmd_mux_choose value: %d. "
        "Publishing zero velocity.",
        static_cast<int>(cmd_mux_));
    }

    const double distance_limit =
      (cmd_mux_ == 2) ?
      kDockDistanceLimit :
      kNormalDistanceLimit;

    const auto cmd_vel_safety =
      safetyControl(
      cmd_vel_,
      distance_limit);

    cmd_vel_pub_->publish(
      cmd_vel_safety);
  }
};


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
    std::make_shared<SafetyManager>());

  rclcpp::shutdown();

  return 0;
}