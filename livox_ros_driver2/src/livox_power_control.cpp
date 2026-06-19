#include "livox_power_control.h"

#include <arpa/inet.h>

#include <sstream>
#include <utility>

namespace livox_ros {

LivoxPowerControl::LivoxPowerControl(rclcpp::Node * node)
: node_(node)
{
  lidar_ips_ = node_->declare_parameter<std::vector<std::string>>(
    "livox_control.lidar_ips",
    {
      "192.168.1.100",
      "192.168.1.101",
    });


    sleep_service_ = node_->create_service<std_srvs::srv::SetBool>(
        "/livox/set_sleep",
        [this](
            const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
            std::shared_ptr<std_srvs::srv::SetBool::Response> response)
        {
            const LivoxLidarWorkMode mode =
            request->data
                ? static_cast<LivoxLidarWorkMode>(0x02)   // MID-360 IDLE
                : static_cast<LivoxLidarWorkMode>(0x01); // MID-360 SAMPLING

            const std::string command_name =
            request->data ? "idle" : "sampling";

            response->success = sendToAll(
            command_name,
            [mode](
                uint32_t handle,
                LivoxLidarAsyncControlCallback callback,
                void * client_data)
            {
                return SetLivoxLidarWorkMode(
                handle,
                mode,
                callback,
                client_data);
            },
            response->message);
        });

  normal_service_ = node_->create_service<std_srvs::srv::Trigger>(
    "/livox/set_normal",
    [this](
      const std::shared_ptr<std_srvs::srv::Trigger::Request>,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
      response->success = sendToAll(
        "normal",
        [](
          uint32_t handle,
          LivoxLidarAsyncControlCallback callback,
          void * client_data)
        {
          return SetLivoxLidarWorkMode(
            handle,
            kLivoxLidarNormal,
            callback,
            client_data);
        },
        response->message);
    });

  pointcloud_service_ = node_->create_service<std_srvs::srv::SetBool>(
    "/livox/set_pointcloud",
    [this](
      const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
      std::shared_ptr<std_srvs::srv::SetBool::Response> response)
    {
      const bool enable = request->data;

      const std::string command_name =
        enable ? "pointcloud_enable" : "pointcloud_disable";

      response->success = sendToAll(
        command_name,
        [enable](
          uint32_t handle,
          LivoxLidarAsyncControlCallback callback,
          void * client_data)
        {
          if (enable) {
            return EnableLivoxLidarPointSend(
              handle,
              callback,
              client_data);
          }

          return DisableLivoxLidarPointSend(
            handle,
            callback,
            client_data);
        },
        response->message);
    });

  RCLCPP_INFO(
    node_->get_logger(),
    "Livox power control services started.");

  for (const auto & ip : lidar_ips_) {
    RCLCPP_INFO(
      node_->get_logger(),
      "Livox control target: %s",
      ip.c_str());
  }
}

bool LivoxPowerControl::ipToHandle(
  const std::string & ip,
  uint32_t & handle)
{
  in_addr address {};

  if (inet_pton(AF_INET, ip.c_str(), &address) != 1) {
    return false;
  }

  handle = address.s_addr;
  return true;
}

bool LivoxPowerControl::sendToAll(
  const std::string & command_name,
  const SdkCommand & command,
  std::string & result_message)
{
  size_t accepted_count = 0;
  std::ostringstream error_stream;

  for (const auto & ip : lidar_ips_) {
    uint32_t handle = 0;

    if (!ipToHandle(ip, handle)) {
      error_stream
        << "[" << ip << "] invalid IPv4 address; ";

      RCLCPP_ERROR(
        node_->get_logger(),
        "[%s] Invalid IPv4 address.",
        ip.c_str());

      continue;
    }

    auto * context = new CommandContext {
      ip,
      command_name,
    };

    const livox_status status =
      command(handle, commandCallback, context);

    if (status != kLivoxLidarStatusSuccess) {
      delete context;

      error_stream
        << "[" << ip << "] SDK call rejected, status="
        << status << "; ";

      RCLCPP_ERROR(
        node_->get_logger(),
        "[%s] Failed to request '%s'. SDK status=%d",
        ip.c_str(),
        command_name.c_str(),
        status);

      continue;
    }

    ++accepted_count;

    RCLCPP_INFO(
      node_->get_logger(),
      "[%s] Requested '%s'. Waiting for asynchronous ACK.",
      ip.c_str(),
      command_name.c_str());
  }

  std::ostringstream result_stream;

  result_stream
    << "Command '" << command_name << "' accepted for "
    << accepted_count << " / " << lidar_ips_.size()
    << " LiDAR(s).";

  if (!error_stream.str().empty()) {
    result_stream << " Errors: " << error_stream.str();
  }

  result_message = result_stream.str();

  return accepted_count == lidar_ips_.size();
}

void LivoxPowerControl::commandCallback(
  livox_status status,
  uint32_t handle,
  LivoxLidarAsyncControlResponse * response,
  void * client_data)
{
  std::unique_ptr<CommandContext> context(
    static_cast<CommandContext *>(client_data));

  const auto logger =
    rclcpp::get_logger("livox_power_control");

  if (!context) {
    RCLCPP_ERROR(
      logger,
      "Received Livox ACK without a command context. handle=%u",
      handle);

    return;
  }

  if (status != kLivoxLidarStatusSuccess) {
    RCLCPP_ERROR(
      logger,
      "[%s] Command '%s' failed. SDK status=%d, handle=%u",
      context->ip.c_str(),
      context->command.c_str(),
      status,
      handle);

    return;
  }

  if (response == nullptr) {
    RCLCPP_ERROR(
      logger,
      "[%s] Command '%s' returned a null response. handle=%u",
      context->ip.c_str(),
      context->command.c_str(),
      handle);

    return;
  }

  if (response->ret_code != 0 || response->error_key != 0) {
    RCLCPP_ERROR(
      logger,
      "[%s] Command '%s' rejected by LiDAR. "
      "ret_code=%u, error_key=%u, handle=%u",
      context->ip.c_str(),
      context->command.c_str(),
      response->ret_code,
      response->error_key,
      handle);

    return;
  }

  RCLCPP_INFO(
    logger,
    "[%s] Command '%s' completed successfully. handle=%u",
    context->ip.c_str(),
    context->command.c_str(),
    handle);
}

}  // namespace livox_ros