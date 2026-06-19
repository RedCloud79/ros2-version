import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    multi_robot_name = LaunchConfiguration('multi_robot_name')

    description_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('irop_description'),
                'launch',
                'description.launch.py',
            )
        ),
        launch_arguments={
            'multi_robot_name': multi_robot_name,
        }.items(),
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'multi_robot_name',
            default_value='',
            description='Optional TF frame prefix',
        ),

        description_launch,
    ])