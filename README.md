---
Finished <<< pointcloud_concatenate [1min 5s]
Starting >>> livox_ros_driver2
--- stderr: irop_description                
CMake Warning:
  Manually-specified variables were not used by the project:

    HUMBLE_ROS
    ROS_EDITION


CMake Error at cmake_install.cmake:46 (file):
  file INSTALL cannot find "/home/irop/ros2_ws/src/irop_description/config":
  No such file or directory.


---
Failed   <<< irop_description [8.41s, exited with code 1]
--- stderr: ndt_omp
** WARNING ** io features related to pcap will be disabled
In this package, headers install destination is set to `include` by ament_auto_package. It is recommended to install `include/ndt_omp` instead and will be the default behavior of ament_auto_package from ROS 2 Kilted Kaiju. On distributions before Kilted, ament_auto_package behaves the same way when you use USE_SCOPED_HEADER_INSTALL_DIR option.
CMake Warning:
  Manually-specified variables were not used by the project:

    CATKIN_INSTALL_INTO_PREFIX_ROOT
    HUMBLE_ROS
    ROS_EDITION


---
Finished <<< ndt_omp [1min 12s]
Aborted  <<< irop_camera [9.92s]                       
Aborted  <<< nav2_voxel_grid [1min 20s]                
Aborted  <<< hdl_global_localization [1min 50s]        
Aborted  <<< fast_gicp [1min 50s]                       
Aborted  <<< livox_ros_driver2 [1min 11s]               
Aborted  <<< nav2_msgs [2min 44s]           

Summary: 6 packages finished [2min 50s]
  1 package failed: irop_description
  6 packages aborted: fast_gicp hdl_global_localization irop_camera livox_ros_driver2 nav2_msgs nav2_voxel_grid
  12 packages had stderr output: costmap_converter_msgs fast_gicp hdl_global_localization irop_camera irop_description nav2_common nav2_msgs nav2_voxel_grid ndt_omp pointcloud_concatenate vectornav_msgs yhs_can_interfaces
  25 packages not processed
