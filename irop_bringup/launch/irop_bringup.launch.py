import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    GroupAction
)
from launch.conditions import IfCondition
from launch.launch_description_sources import (
    # FrontendLaunchDescriptionSource,
    PythonLaunchDescriptionSource,
)
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def include_python_launch(
    package_name,
    launch_file_name,
    arguments=None,
    launch_directory='launch',
):
    launch_file_path = os.path.join(
        get_package_share_directory(package_name),
        launch_directory,
        launch_file_name,
    )

    return GroupAction([IncludeLaunchDescription(
        PythonLaunchDescriptionSource(launch_file_path),
        launch_arguments=(arguments or {}).items(),
    )])


# def include_frontend_launch(
#     package_name,
#     launch_file_name,
#     arguments=None,
#     launch_directory='launch',
# ):
#     launch_file_path = os.path.join(
#         get_package_share_directory(package_name),
#         launch_directory,
#         launch_file_name,
#     )

#     return GroupAction([IncludeLaunchDescription(
#         FrontendLaunchDescriptionSource(launch_file_path),
#         launch_arguments=(arguments or {}).items(),
#     )])


def generate_launch_description():
    nav2_bringup_share = get_package_share_directory('nav2_bringup')
    irop_bringup_share = get_package_share_directory('irop_bringup')

    multi_robot_name = LaunchConfiguration('multi_robot_name')
    map_name = LaunchConfiguration('map_name')
    map_pcd = LaunchConfiguration('map_pcd')
    map_metadata = LaunchConfiguration('map_metadata')
    ndt_input_pointcloud = LaunchConfiguration('ndt_input_pointcloud')
    use_rviz = LaunchConfiguration('use_rviz')

    default_map_pcd_path = os.path.join(
        nav2_bringup_share,
        'maps_3d',
        'gasan_office_xyzi.pcd',
    )

    default_map_metadata_path = ''

    rviz_config_file = os.path.join(
        irop_bringup_share,
        'rviz',
        'irop_default.rviz',
    )

    declare_multi_robot_name = DeclareLaunchArgument(
        'multi_robot_name',
        default_value='',
        description='Optional robot namespace or TF prefix',
    )

    declare_map_name = DeclareLaunchArgument(
        'map_name',
        default_value='gasan_office.yaml',
        description='Name of the 2D map YAML file in nav2_bringup/maps_2d',
    )

    declare_map_pcd = DeclareLaunchArgument(
        'map_pcd',
        default_value=default_map_pcd_path,
        description='Full path to the Autoware 3D pointcloud map PCD file',
    )

    declare_map_metadata = DeclareLaunchArgument(
        'map_metadata',
        default_value=default_map_metadata_path,
        description='Optional metadata YAML file for the Autoware pointcloud map',
    )

    declare_ndt_input_pointcloud = DeclareLaunchArgument(
        'ndt_input_pointcloud',
        default_value='/rslidar_points_xyzi',
        description='PointCloud2 topic used by NDT scan matcher',
    )

    declare_use_rviz = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='Launch RViz2 automatically',
    )

    irop_robot_launch = include_python_launch(
        package_name='irop_bringup',
        launch_file_name='irop_robot.launch.py',
    )

    irop_remote_launch = include_python_launch(
        package_name='irop_bringup',
        launch_file_name='irop_remote.launch.py',
        arguments={
            'multi_robot_name': multi_robot_name,
        },
    )

    control_robot_node = Node(
        package='irop_bringup',
        executable='control_robot_node',
        name='control_robot_node',
        output='screen',
    )

    mid360_launch = include_python_launch(
        package_name='livox_ros_driver2',
        launch_directory='launch_ROS2',
        launch_file_name='msg_MID360.launch.py',
    )

    # fast_lio_launch = include_python_launch(
    #     package_name='fast_lio',
    #     launch_file_name='odom_slam.launch.py',
    # )

    nav2_params_file = os.path.join(
        irop_bringup_share,
        'config',
        'nav2_params.yaml',
    )

    nav2_launch = include_python_launch(
        package_name='nav2_bringup',
        launch_file_name='navigation_launch.py',
        arguments={
            'map_name': map_name,
            'params_file': nav2_params_file,
            'use_sim_time': 'false',
            'autostart': 'true',
            'use_composition': 'False',
        },
    )


    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=[
            '-d',
            rviz_config_file,
        ],
        condition=IfCondition(use_rviz),
    )

    irop_localization_launch = include_python_launch(
        package_name='irop_bringup',
        launch_file_name='irop_localization.launch.py',
    )

    localization_3d_launch = include_python_launch(
        package_name='hdl_localization',
        launch_file_name='hdl_localization.launch.py',
    )

    return LaunchDescription([
        declare_multi_robot_name,
        declare_map_name,
        declare_map_pcd,
        declare_map_metadata,
        declare_ndt_input_pointcloud,
        declare_use_rviz,

        irop_robot_launch,
        irop_remote_launch,
        control_robot_node,

        mid360_launch,

        # fast_lio_launch,

        nav2_launch,

        rviz_node,
        irop_localization_launch,
        localization_3d_launch
    ])