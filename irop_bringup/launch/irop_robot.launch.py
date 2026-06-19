import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    yhs_can_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('yhs_can_control'),
                'launch',
                'yhs_can_control.launch.py',
            )
        )
    )

    imu_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('irop_bringup'),
                'launch',
                'imu.launch.py',
            )
        )
    )

    return LaunchDescription([
        yhs_can_launch,
        imu_launch,
    ])