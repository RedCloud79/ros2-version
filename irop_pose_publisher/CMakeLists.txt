cmake_minimum_required(VERSION 3.8)

project(irop_pose_publisher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

find_package(ament_cmake REQUIRED)

find_package(rclcpp REQUIRED)

find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

find_package(geometry_msgs REQUIRED)

add_executable(
  robot_pose_publisher
  src/robot_pose_publisher.cpp
)

ament_target_dependencies(
  robot_pose_publisher
  rclcpp
  tf2
  tf2_ros
  geometry_msgs
)

install(
  TARGETS
    robot_pose_publisher
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()