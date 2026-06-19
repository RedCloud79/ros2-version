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
        'MID360_config.json'
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
                'multi_topic': 1,
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

                # MID-360 control service targets
                'livox_control.lidar_ips': [
                    '192.168.1.100',
                    '192.168.1.101',
                ],
            }]
        ),

        Node(
            package='pointcloud_concatenate',
            executable='offset_concatenate_pointclouds_node',
            name='mid360_pointcloud_concatenate',
            output='screen',

            arguments=[
                '--ros-args',
                '--log-level',
                'warn',
            ],

            parameters=[{
                'output_frame': 'livox_frame',

                'input_topics': [
                    '/livox/lidar_192_168_1_100',
                    '/livox/lidar_192_168_1_101',
                ],

                'input_offset_msec': [
                    0.0,
                    0.0,
                ],

                'angle_limit_lidar_index': -1,

                'angle_range_deg': [
                    -180.0,
                    180.0,
                ],
            }],

            remappings=[
                ('output', '/rslidar_points_raw'),
            ]
        ),

        Node(
            package='livox_ros_driver2',
            executable='self_cloud_filter_node',
            name='self_cloud_filter',
            output='screen',
            parameters=[{
                'remove_x': 0.20,
                'remove_z': 0.60,

                # false: preserve the original ROS1 behavior.
                # true : additionally limit the removed region on Y axis.
                'use_y_limit': True,
                'remove_y': 0.15,
            }],
            remappings=[
                ('cloud_in', '/rslidar_points_raw'),
                ('cloud_out', '/rslidar_points'),
            ]
        ),

        Node(
            package='pointcloud_to_laserscan',
            executable='pointcloud_to_laserscan_node',
            name='cloud_to_scan',
            output='screen',
            parameters=[{
                'target_frame': 'base_link',
                'transform_tolerance': 0.8,

                'min_height': 0.05,
                'max_height': 0.25,

                'range_min': 0.1,
                'range_max': 80.0,

                'angle_min': -3.14159,
                'angle_max': 3.14159,
                'angle_increment': 0.012,

                'scan_time': 0.067,
                'use_inf': False,
            }],
            remappings=[
                ('cloud_in', '/rslidar_points'),
                ('scan', '/scan'),
            ]
        ),
    ])