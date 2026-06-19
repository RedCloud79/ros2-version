import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import (
    GroupAction,
    IncludeLaunchDescription,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    irop_bringup_share = get_package_share_directory(
        'irop_bringup'
    )

    microstrain_driver_share = get_package_share_directory(
        'microstrain_inertial_driver'
    )

    imu_params_file = os.path.join(
        irop_bringup_share,
        'config',
        '3dm_cv7_ahrs.yaml',
    )

    microstrain_imu_launch = GroupAction(
        scoped=True,
        forwarding=False,
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(
                        microstrain_driver_share,
                        'launch',
                        'microstrain_launch.py',
                    )
                ),
                launch_arguments={
                    'params_file': imu_params_file,
                }.items(),
            ),
        ],
    )

    return LaunchDescription([
        microstrain_imu_launch,
    ])