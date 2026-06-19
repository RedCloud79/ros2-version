from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'rtsp_url',
            default_value='rtsp://username:password@ip_address:port/stream'
        ),
        DeclareLaunchArgument(
            'frame_id',
            default_value='camera_frame'
        ),
        DeclareLaunchArgument(
            'fps',
            default_value='30.0'
        ),
        Node(
            package='irop_camera',
            executable='irop_camera',
            name='irop_camera',
            parameters=[{
                'rtsp_url': LaunchConfiguration('rtsp_url'),
                'frame_id': LaunchConfiguration('frame_id'),
                'fps': LaunchConfiguration('fps'),
            }]
        ),
    ])