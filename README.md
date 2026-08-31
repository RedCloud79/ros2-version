Finished <<< nav2_bringup [14.1s]
--- stderr: hdl_global_localization          
CMake Deprecation Warning at /opt/ros/humble/share/rosidl_cmake/cmake/rosidl_target_interfaces.cmake:32 (message):
  Use rosidl_get_typesupport_target() and target_link_libraries() instead of
  rosidl_target_interfaces()
Call Stack (most recent call first):
  CMakeLists.txt:81 (rosidl_target_interfaces)


CMake Deprecation Warning at /opt/ros/humble/share/rosidl_cmake/cmake/rosidl_target_interfaces.cmake:32 (message):
  Use rosidl_get_typesupport_target() and target_link_libraries() instead of
  rosidl_target_interfaces()
Call Stack (most recent call first):
  CMakeLists.txt:90 (rosidl_target_interfaces)


In this package, headers install destination is set to `include` by ament_auto_package. It is recommended to install `include/hdl_global_localization` instead and will be the default behavior of ament_auto_package from ROS 2 Kilted Kaiju. On distributions before Kilted, ament_auto_package behaves the same way when you use USE_SCOPED_HEADER_INSTALL_DIR option.
CMake Warning:
  Manually-specified variables were not used by the project:

    CATKIN_INSTALL_INTO_PREFIX_ROOT
    HUMBLE_ROS
    ROS_EDITION


---
Finished <<< hdl_global_localization [2min 1s]
Starting >>> hdl_localization
--- stderr: hdl_localization                
** WARNING ** io features related to pcap will be disabled
CMake Error at CMakeLists.txt:47 (find_package):
  By not providing "Findfast_gicp.cmake" in CMAKE_MODULE_PATH this project
  has asked CMake to find a package configuration file provided by
  "fast_gicp", but CMake did not find one.

  Could not find a package configuration file provided by "fast_gicp" with
  any of the following names:

    fast_gicpConfig.cmake
    fast_gicp-config.cmake

  Add the installation prefix of "fast_gicp" to CMAKE_PREFIX_PATH or set
  "fast_gicp_DIR" to a directory containing one of the above files.  If
  "fast_gicp" provides a separate development package or SDK, be sure it has
  been installed.


---
Failed   <<< hdl_localization [5.08s, exited with code 1]
Aborted  <<< irop_bringup [30.4s]                      
Aborted  <<< livox_ros_driver2 [1min 38s]               
Aborted  <<< livox_sdk2 [2min 10s]                      
Aborted  <<< vectornav [1min 32s]                       
Aborted  <<< nav2_msgs [3min 21s]           

Summary: 14 packages finished [3min 25s]
  1 package failed: hdl_localization
  5 packages aborted: irop_bringup livox_ros_driver2 livox_sdk2 nav2_msgs vectornav
  19 packages had stderr output: costmap_converter_msgs fast_gicp hdl_global_localization hdl_localization irop_bringup irop_camera irop_description irop_pose_publisher irop_safety_manager livox_sdk2 nav2_bringup nav2_common nav2_msgs nav2_voxel_grid ndt_omp pointcloud_concatenate vectornav vectornav_msgs yhs_can_interfaces
  18 packages not processed
