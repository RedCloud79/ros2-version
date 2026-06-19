import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    config_path = os.path.join(
        get_package_share_directory('livox_ros_driver2'),
        'config',
        'MID360_config_mapping.json'
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'publish_freq',
            default_value='15.0',
            description='Livox PointCloud publish frequency'
        ),

        DeclareLaunchArgument(
            'msg_frame_id',
            default_value='livox_frame',
            description='Frame ID for Livox messages'
        ),

        Node(
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher2',
            output='screen',
            parameters=[{
                'xfer_format': 0,
                'multi_topic': 0,
                'data_src': 0,

                'publish_freq': ParameterValue(
                    LaunchConfiguration('publish_freq'),
                    value_type=float
                ),

                'output_data_type': 0,
                'user_config_path': config_path,
                'frame_id': LaunchConfiguration('msg_frame_id'),

                'enable_lidar_bag': False,
                'enable_imu_bag': False,

                'use_ros_time': True,
                'ros_time_override': True,

                'livox_control.lidar_ips': [
                    '192.168.1.101',
                ],
            }]
        ),
    ])