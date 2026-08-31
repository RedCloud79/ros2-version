Starting >>> vectornav_msgs
--- stderr: nav2_common               
CMake Warning:
  Manually-specified variables were not used by the project:

    HUMBLE_ROS
    ROS_EDITION


---
Finished <<< nav2_common [5.54s]
Starting >>> nav2_msgs
--- stderr: hdl_global_localization    
CMake Error at /opt/ros/humble/share/rosidl_cmake/cmake/rosidl_generate_interfaces.cmake:229 (message):
  Packages installing interfaces must include
  '<member_of_group>rosidl_interface_packages</member_of_group>' in their
  package.xml
Call Stack (most recent call first):
  CMakeLists.txt:44 (rosidl_generate_interfaces)


---
Failed   <<< hdl_global_localization [10.3s, exited with code 1]
Aborted  <<< costmap_converter_msgs [11.0s]       
Aborted  <<< nav2_msgs [9.78s]                    
Aborted  <<< pointcloud_concatenate [25.7s]       
Aborted  <<< vectornav_msgs [41.7s]               
Aborted  <<< yhs_can_interfaces [44.0s]           
Aborted  <<< ndt_omp [52.1s]                      
Aborted  <<< fast_gicp [1min 26s]          

Summary: 1 package finished [1min 27s]
  1 package failed: hdl_global_localization
  7 packages aborted: costmap_converter_msgs fast_gicp nav2_msgs ndt_omp pointcloud_concatenate vectornav_msgs yhs_can_interfaces
  9 packages had stderr output: costmap_converter_msgs fast_gicp hdl_global_localization nav2_common nav2_msgs ndt_omp pointcloud_concatenate vectornav_msgs yhs_can_interfaces
  29 packages not processed
