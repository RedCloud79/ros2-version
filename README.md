Starting >>> nav2_smac_planner
[Processing: costmap_converter, nav2_behaviors, nav2_bt_navigator, nav2_constrained_smoother, nav2_controller, nav2_navfn_planner, nav2_planner, nav2_smac_planner]
--- stderr: costmap_converter                
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/src/costmap_to_dynamic_obstacles/blob_detector.cpp: In static member function ‘static cv::Ptr<BlobDetector> BlobDetector::create(const cv::SimpleBlobDetector::Params&)’:
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/src/costmap_to_dynamic_obstacles/blob_detector.cpp:9:56: error: invalid new-expression of abstract class type ‘BlobDetector’
    9 |   return cv::Ptr<BlobDetector> (new BlobDetector(params)); // compatibility with older versions
      |                                                        ^
In file included from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/src/costmap_to_dynamic_obstacles/blob_detector.cpp:1:
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_to_dynamic_obstacles/blob_detector.h:60:7: note:   because the following virtual functions are pure within ‘BlobDetector’:
   60 | class BlobDetector : public cv::SimpleBlobDetector
      |       ^~~~~~~~~~~~
In file included from /usr/include/opencv4/opencv2/features2d/features2d.hpp:48,
                 from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_to_dynamic_obstacles/blob_detector.h:48,
                 from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/src/costmap_to_dynamic_obstacles/blob_detector.cpp:1:
/usr/include/opencv4/opencv2/features2d.hpp:789:24: note:     ‘virtual void cv::SimpleBlobDetector::setParams(const cv::SimpleBlobDetector::Params&)’
  789 |   CV_WRAP virtual void setParams(const SimpleBlobDetector::Params& params ) = 0;
      |                        ^~~~~~~~~
/usr/include/opencv4/opencv2/features2d.hpp:790:46: note:     ‘virtual cv::SimpleBlobDetector::Params cv::SimpleBlobDetector::getParams() const’
  790 |   CV_WRAP virtual SimpleBlobDetector::Params getParams() const = 0;
      |                                              ^~~~~~~~~
gmake[2]: *** [CMakeFiles/costmap_converter.dir/build.make:160: CMakeFiles/costmap_converter.dir/src/costmap_to_dynamic_obstacles/blob_detector.cpp.o] Error 1
gmake[2]: *** Waiting for unfinished jobs....
In file included from /opt/ros/humble/include/rclcpp/rclcpp/logging.hpp:24,
                 from /opt/ros/humble/include/rclcpp/rclcpp/client.hpp:40,
                 from /opt/ros/humble/include/rclcpp/rclcpp/callback_group.hpp:24,
                 from /opt/ros/humble/include/rclcpp/rclcpp/any_executable.hpp:20,
                 from /opt/ros/humble/include/rclcpp/rclcpp/memory_strategy.hpp:25,
                 from /opt/ros/humble/include/rclcpp/rclcpp/memory_strategies.hpp:18,
                 from /opt/ros/humble/include/rclcpp/rclcpp/executor_options.hpp:20,
                 from /opt/ros/humble/include/rclcpp/rclcpp/executor.hpp:37,
                 from /opt/ros/humble/include/rclcpp/rclcpp/executors/multi_threaded_executor.hpp:25,
                 from /opt/ros/humble/include/rclcpp/rclcpp/executors.hpp:21,
                 from /opt/ros/humble/include/rclcpp/rclcpp/rclcpp.hpp:155,
                 from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_converter_interface.h:46,
                 from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_to_lines_ransac.h:42,
                 from /home/irop/ros2_ws/src/costmap_converter/costmap_converter/src/costmap_to_lines_ransac.cpp:39:
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_converter_interface.h: In member function ‘void costmap_converter::BaseCostmapToPolygons::startWorker(rclcpp::GenericRate<std::chrono::_V2::system_clock>::SharedPtr, nav2_costmap_2d::Costmap2D*, bool)’:
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_converter_interface.h:197:41: warning: too many arguments for format [-Wformat-extra-args]
  197 |         RCLCPP_DEBUG(nh_->get_logger(), "costmap_converter", "Spinning up a thread for the CostmapToPolygons plugin");
      |                                         ^~~~~~~~~~~~~~~~~~~
/home/irop/ros2_ws/src/costmap_converter/costmap_converter/include/costmap_converter/costmap_converter_interface.h: In member function ‘void costmap_converter::BaseCostmapToDynamicObstacles::loadStaticCostmapConverterPlugin(const string&, rclcpp::Node::SharedPtr)’:
