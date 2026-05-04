import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, PushRosNamespace

def generate_launch_description():
    # Paths
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')
    pkg_dug_core = get_package_share_directory('dug_core')
    pkg_dug_comm = get_package_share_directory('dug_communication')
    
    # Launch arguments
    drone_count_arg = DeclareLaunchArgument(
        'drone_count',
        default_value='5',
        description='Number of drones to spawn'
    )
    
    # Base Launch Description
    ld = LaunchDescription()
    ld.add_action(drone_count_arg)

    # Gazebo Server & Client (Optional, usually run once)
    ld.add_action(IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(pkg_gazebo_ros, 'launch', 'gzserver.launch.py'))
    ))
    ld.add_action(IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(pkg_gazebo_ros, 'launch', 'gzclient.launch.py'))
    ))

    # Mesh Monitor (Singleton)
    ld.add_action(Node(
        package='dug_communication',
        executable='mesh_monitor',
        name='mesh_monitor',
        output='screen'
    ))

    # Spawning UAV Nodes
    # Note: LaunchConfiguration is not directly iterable in Python logic during generation.
    # We usually use a fixed maximum or a script that generates launch files.
    # For demonstration, we'll spawn a few nodes manually or assume 5.
    
    for i in range(5):
        uav_id = i + 1
        namespace = f'uav_{uav_id}'
        
        ld.add_action(GroupAction([
            PushRosNamespace(namespace),
            Node(
                package='dug_core',
                executable='uav_node',
                name='uav_node',
                parameters=[{'uav_id': uav_id}],
                output='screen'
            ),
            # Vision node for each UAV
            Node(
                package='dug_vision',
                executable='target_detector',
                name='target_detector',
                output='screen'
            )
        ]))

    return ld
