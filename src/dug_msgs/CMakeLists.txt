cmake_minimum_required(VERSION 3.8)
project(dug_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/SwarmState.msg"
  "msg/TargetInfo.msg"
  "msg/FormationState.msg"
  "msg/MissionCommand.msg"
  "msg/ObstacleDistance.msg"
  "msg/KamikazeCommand.msg"
  DEPENDENCIES geometry_msgs std_msgs
)

ament_package()


