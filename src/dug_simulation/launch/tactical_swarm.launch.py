import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, GroupAction, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, PushRosNamespace

def launch_setup(context, *args, **kwargs):
    # Read the drone_count argument from context
    drone_count = int(LaunchConfiguration('drone_count').perform(context))
    
    actions = []
    
    for i in range(drone_count):
        uav_id = i + 1
        namespace = f'uav_{uav_id}'
        
        actions.append(GroupAction([
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
            ),
            # VSLAM node for each UAV
            Node(
                package='dug_vision',
                executable='vslam_node',
                name='vslam_node',
                output='screen'
            )
        ]))
        
    return actions

def generate_launch_description():
    # Paths
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')
    
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

    # Dynamic Spawning via OpaqueFunction
    ld.add_action(OpaqueFunction(function=launch_setup))

    return ld
