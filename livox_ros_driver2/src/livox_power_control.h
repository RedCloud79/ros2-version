#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/set_bool.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "livox_lidar_api.h"

namespace livox_ros {

class LivoxPowerControl
{
public:
  explicit LivoxPowerControl(rclcpp::Node * node);

private:
  struct CommandContext
  {
    std::string ip;
    std::string command;
  };

  using SdkCommand = std::function<livox_status(
    uint32_t,
    LivoxLidarAsyncControlCallback,
    void *)>;

  static void commandCallback(
    livox_status status,
    uint32_t handle,
    LivoxLidarAsyncControlResponse * response,
    void * client_data);

  bool sendToAll(
    const std::string & command_name,
    const SdkCommand & command,
    std::string & result_message);

  static bool ipToHandle(
    const std::string & ip,
    uint32_t & handle);

  rclcpp::Node * node_;
  std::vector<std::string> lidar_ips_;

  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr sleep_service_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr normal_service_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr pointcloud_service_;
};

}  // namespace livox_ros