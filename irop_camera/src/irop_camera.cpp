#include "cv_bridge/cv_bridge.h"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/header.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("irop_camera", options);
  image_transport::ImageTransport it(node);
  image_transport::Publisher pub = it.advertise("camera/image", 1);

  node->declare_parameter("rtsp_url", rclcpp::PARAMETER_STRING);
  node->declare_parameter("frame_id", rclcpp::PARAMETER_STRING);
  node->declare_parameter("fps", rclcpp::PARAMETER_DOUBLE);

  std::string rtsp_url;
  std::string frame_id;
  double fps;

  try {
    rtsp_url = node->get_parameter("rtsp_url").get_parameter_value().get<std::string>();
    frame_id = node->get_parameter("frame_id").get_parameter_value().get<std::string>();
    fps = node->get_parameter("fps").get_parameter_value().get<double>();
  } catch (const rclcpp::exceptions::ParameterUninitializedException & e) {
    RCLCPP_ERROR(node->get_logger(), "Failed to get parameters: %s", e.what());
    return 1;
  }

  cv::VideoCapture cap(rtsp_url);

  if (!cap.isOpened()) {
    return 1;
  }

  cv::Mat frame;
  std_msgs::msg::Header hdr;
  hdr.frame_id = frame_id;
  sensor_msgs::msg::Image::SharedPtr msg;

  rclcpp::WallRate loop_rate(fps);
  while (rclcpp::ok()) {
    cap >> frame;
    if (!frame.empty()) {
      msg = cv_bridge::CvImage(hdr, "bgr8", frame).toImageMsg();
      pub.publish(msg);
      cv::waitKey(1);
    }

    rclcpp::spin_some(node);
    loop_rate.sleep();
  }

  return 0;
}