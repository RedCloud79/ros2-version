cmake_minimum_required(VERSION 3.8)

project(irop_safety_manager)

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
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(
  safety_manager
  src/safety_manager.cpp
)

ament_target_dependencies(
  safety_manager
  rclcpp
  geometry_msgs
  sensor_msgs
  std_msgs
)

install(
  TARGETS
    safety_manager
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()