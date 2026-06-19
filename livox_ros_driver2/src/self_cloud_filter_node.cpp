#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>

class SelfCloudFilterNode : public rclcpp::Node
{
public:
  SelfCloudFilterNode()
  : Node("self_cloud_filter_node")
  {
    remove_x_ = this->declare_parameter<double>("remove_x", 0.15);
    remove_z_ = this->declare_parameter<double>("remove_z", 0.25);

    use_y_limit_ = this->declare_parameter<bool>("use_y_limit", false);
    remove_y_ = this->declare_parameter<double>("remove_y", 0.35);

    auto qos = rclcpp::SensorDataQoS().keep_last(1);

    pub_filtered_ =
      this->create_publisher<sensor_msgs::msg::PointCloud2>(
        "cloud_out",
        qos
      );

    sub_cloud_ =
      this->create_subscription<sensor_msgs::msg::PointCloud2>(
        "cloud_in",
        qos,
        std::bind(
          &SelfCloudFilterNode::callback,
          this,
          std::placeholders::_1
        )
      );

    RCLCPP_INFO(
      this->get_logger(),
      "Self cloud filter started: "
      "remove_x=+/-%.3f, remove_z=+/-%.3f, "
      "use_y_limit=%s, remove_y=+/-%.3f",
      remove_x_,
      remove_z_,
      use_y_limit_ ? "true" : "false",
      remove_y_
    );
  }

private:
  void callback(
    const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg)
  {
    pcl::PointCloud<pcl::PointXYZI> cloud_in;
    pcl::fromROSMsg(*msg, cloud_in);

    if (cloud_in.points.empty()) {
      return;
    }

    pcl::PointCloud<pcl::PointXYZI> cloud_out;
    cloud_out.points.reserve(cloud_in.points.size());

    for (const auto & p : cloud_in.points) {
      if (!std::isfinite(p.x) ||
          !std::isfinite(p.y) ||
          !std::isfinite(p.z)) {
        continue;
      }

      bool is_self_point =
        std::abs(p.x) <= remove_x_ &&
        std::abs(p.z) <= remove_z_;

      if (use_y_limit_) {
        is_self_point =
          is_self_point &&
          std::abs(p.y) <= remove_y_;
      }

      if (!is_self_point) {
        cloud_out.points.push_back(p);
      }
    }

    cloud_out.width =
      static_cast<std::uint32_t>(cloud_out.points.size());

    cloud_out.height = 1;
    cloud_out.is_dense = false;

    sensor_msgs::msg::PointCloud2 cloud_msg;
    pcl::toROSMsg(cloud_out, cloud_msg);

    // timestamp와 frame_id 유지
    cloud_msg.header = msg->header;

    pub_filtered_->publish(cloud_msg);
  }

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr
    pub_filtered_;

  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr
    sub_cloud_;

  double remove_x_;
  double remove_y_;
  double remove_z_;
  bool use_y_limit_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
    std::make_shared<SelfCloudFilterNode>()
  );

  rclcpp::shutdown();

  return 0;
}