from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="irop_bringup",
                executable="control_robot_node",
                name="control_robot_node",
                output="screen",
            ),
        ]
    )