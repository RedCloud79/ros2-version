inished <<< costmap_converter_msgs [16.1s]
Starting >>> nav2_voxel_grid
--- stderr: pointcloud_concatenate     
In this package, headers install destination is set to `include` by ament_auto_package. It is recommended to install `include/pointcloud_concatenate` instead and will be the default behavior of ament_auto_package from ROS 2 Kilted Kaiju. On distributions before Kilted, ament_auto_package behaves the same way when you use USE_SCOPED_HEADER_INSTALL_DIR option.
CMake Warning:
  Manually-specified variables were not used by the project:

    HUMBLE_ROS
    ROS_EDITION


In file included from /home/irop/ros2_ws/src/pointcloud_concatenate/src/pointcloud_concatenate/offset_concatenate_pointclouds.cpp:15:
/home/irop/ros2_ws/src/pointcloud_concatenate/include/pointcloud_concatenate/offset_concatenate_pointclouds.hpp:30:10: fatal error: point_cloud_msg_wrapper/point_cloud_msg_wrapper.hpp: No such file or directory
   30 | #include <point_cloud_msg_wrapper/point_cloud_msg_wrapper.hpp>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
gmake[2]: *** [CMakeFiles/pointcloud_concatenate_node.dir/build.make:76: CMakeFiles/pointcloud_concatenate_node.dir/src/pointcloud_concatenate/offset_concatenate_pointclouds.cpp.o] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:139: CMakeFiles/pointcloud_concatenate_node.dir/all] Error 2
gmake[1]: *** Waiting for unfinished jobs....
gmake: *** [Makefile:146: all] Error 2
---
Failed   <<< pointcloud_concatenate [29.2s, exited with code 2]
Aborted  <<< nav2_voxel_grid [39.0s]              
Aborted  <<< vectornav_msgs [55.1s]
Aborted  <<< yhs_can_interfaces [57.7s]           
Aborted  <<< ndt_omp [1min 6s]                        
Aborted  <<< fast_gicp [1min 43s]                      
Aborted  <<< hdl_global_localization [1min 51s]        
Aborted  <<< nav2_msgs [2min 19s]          

Summary: 2 packages finished [2min 26s]
  1 package failed: pointcloud_concatenate
  7 packages aborted: fast_gicp hdl_global_localization nav2_msgs nav2_voxel_grid ndt_omp vectornav_msgs yhs_can_interfaces
  10 packages had stderr output: costmap_converter_msgs fast_gicp hdl_global_localization nav2_common nav2_msgs nav2_voxel_grid ndt_omp pointcloud_concatenate vectornav_msgs yhs_can_interfaces
  28 packages not processed
