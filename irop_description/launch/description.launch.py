import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    multi_robot_name = LaunchConfiguration('multi_robot_name')

    xacro_file = os.path.join(
        get_package_share_directory('irop_description'),
        'urdf',
        'fw-mini.urdf.xacro',
    )

    robot_description = ParameterValue(
        Command([
            'xacro ',
            xacro_file,
        ]),
        value_type=str,
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='log',
        parameters=[{
            'robot_description': robot_description,
            'publish_frequency': 50.0,

            # ROS1의 tf_prefix에 대응
            'frame_prefix': multi_robot_name,
        }],
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'multi_robot_name',
            default_value='',
            description='Optional TF frame prefix for multi-robot operation',
        ),

        robot_state_publisher_node,
    ])