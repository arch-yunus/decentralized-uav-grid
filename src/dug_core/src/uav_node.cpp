#include "dug_core/uav_node.hpp"

namespace dug_core
{

UavNode::UavNode()
: Node("uav_node"), is_leader_(false), battery_level_(100.0)
{
  // Parameters
  this->declare_parameter("uav_id", 0);
  uav_id_ = this->get_parameter("uav_id").as_int();

  RCLCPP_INFO(this->get_logger(), "Initializing UAV Node with ID: %d", uav_id_);

  // Subscriptions
  state_sub_ = this->create_subscription<mavros_msgs::msg::State>(
    "mavros/state", 10, std::bind(&UavNode::state_callback, this, std::placeholders::_1));
  
  pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
    "mavros/local_position/pose", 10, std::bind(&UavNode::pose_callback, this, std::placeholders::_1));

  swarm_sub_ = this->create_subscription<dug_msgs::msg::SwarmState>(
    "/swarm/status", 10, std::bind(&UavNode::swarm_callback, this, std::placeholders::_1));

  target_sub_ = this->create_subscription<dug_msgs::msg::TargetInfo>(
    "/swarm/targets", 10, std::bind(&UavNode::target_callback, this, std::placeholders::_1));

  formation_sub_ = this->create_subscription<dug_msgs::msg::FormationState>(
    "/swarm/formation", 10, std::bind(&UavNode::formation_callback, this, std::placeholders::_1));

  obstacle_sub_ = this->create_subscription<dug_msgs::msg::ObstacleDistance>(
    "/swarm/obstacles", 10, std::bind(&UavNode::obstacle_callback, this, std::placeholders::_1));

  mission_sub_ = this->create_subscription<dug_msgs::msg::MissionCommand>(
    "/swarm/mission", 10, std::bind(&UavNode::mission_callback, this, std::placeholders::_1));

  vslam_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
    "/swarm/vslam_pose", 10, std::bind(&UavNode::vslam_callback, this, std::placeholders::_1));

  kamikaze_sub_ = this->create_subscription<dug_msgs::msg::KamikazeCommand>(
    "/swarm/kamikaze", 10, std::bind(&UavNode::kamikaze_callback, this, std::placeholders::_1));

  gps_healthy_ = true;
  last_gps_time_ = this->now();
  is_kamikaze_mode_ = false;

  // Publishers
  swarm_pub_ = this->create_publisher<dug_msgs::msg::SwarmState>("/swarm/status", 10);
  formation_pub_ = this->create_publisher<dug_msgs::msg::FormationState>("/swarm/formation", 10);
  cmd_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("mavros/setpoint_position/local", 10);

  // Timer for status publishing and leader selection (1Hz)
  timer_ = this->create_wall_timer(
    std::chrono::seconds(1), std::bind(&UavNode::timer_callback, this));
}

void UavNode::state_callback(const mavros_msgs::msg::State::SharedPtr msg)
{
  current_state_ = *msg;
}

void UavNode::pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
  current_pose_ = *msg;
  last_gps_time_ = this->now();
  gps_healthy_ = true;
}

void UavNode::swarm_callback(const dug_msgs::msg::SwarmState::SharedPtr msg)
{
  if (msg->uav_id != uav_id_) {
    swarm_members_[msg->uav_id] = *msg;
  }
}

void UavNode::target_callback(const dug_msgs::msg::TargetInfo::SharedPtr msg)
{
  // Simple target synchronization
  bool exists = false;
  for (const auto & t : global_targets_) {
    if (t.target_id == msg->target_id) {
      exists = true;
      break;
    }
  }
  if (!exists) {
    global_targets_.push_back(*msg);
    RCLCPP_INFO(this->get_logger(), "New global target synchronized: %s", msg->target_type.c_str());
  }
}

void UavNode::formation_callback(const dug_msgs::msg::FormationState::SharedPtr msg)
{
  current_formation_ = *msg;
}

void UavNode::obstacle_callback(const dug_msgs::msg::ObstacleDistance::SharedPtr msg)
{
  latest_obstacles_ = msg->distances;
}

void UavNode::mission_callback(const dug_msgs::msg::MissionCommand::SharedPtr msg)
{
  current_mission_ = *msg;
  RCLCPP_INFO(this->get_logger(), "New mission received: %s", msg->command.c_str());
}

void UavNode::vslam_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
  vslam_pose_ = *msg;
}

void UavNode::kamikaze_callback(const dug_msgs::msg::KamikazeCommand::SharedPtr msg)
{
  if (msg->armed) {
    is_kamikaze_mode_ = true;
    attack_target_ = msg->target_location;
    payload_manager_.arm();
    RCLCPP_WARN(this->get_logger(), "ATTACK MODE ENABLED! Datalogging Target: [%f, %f]", attack_target_.x, attack_target_.y);
  } else {
    is_kamikaze_mode_ = false;
  }
}

void UavNode::timer_callback()
{
  // GPS Health Check
  if ((this->now() - last_gps_time_).seconds() > 2.0) {
    if (gps_healthy_) {
      RCLCPP_ERROR(this->get_logger(), "GPS LOST! Switching to Visual SLAM (Phase 4 Logic)");
      gps_healthy_ = false;
    }
    // Failover: replace current pose with SLAM pose
    current_pose_ = vslam_pose_;
  }

  // Update and publish own status
  auto msg = dug_msgs::msg::SwarmState();
  msg.uav_id = uav_id_;
  msg.is_leader = is_leader_;
  msg.battery_percentage = battery_level_;
  msg.current_pose = current_pose_;
  
  swarm_pub_->publish(msg);

  // Perform dynamic leader selection
  select_leader();

  // If leader, publish formation command
  if (is_leader_) {
    auto form_msg = dug_msgs::msg::FormationState();
    form_msg.leader_id = uav_id_;
    form_msg.formation_center = current_pose_.pose.position;
    form_msg.formation_type = "Diamond";
    formation_pub_->publish(form_msg);
  }

  // Execute formation control
  perform_formation_control();
}

void UavNode::perform_formation_control()
{
  if (is_leader_ && !is_kamikaze_mode_) return; 

  if (is_kamikaze_mode_) {
    // Attack logic: direct line to target with high speed
    geometry_msgs::msg::PoseStamped cmd;
    cmd.header.stamp = this->now();
    cmd.header.frame_id = "map";
    
    // Dive towards target
    cmd.pose.position = attack_target_;
    
    // In a real scenario, we would use velocity setpoints, 
    // but here we publish the target as a setpoint.
    cmd_pose_pub_->publish(cmd);
    
    // Simulate impact check
    float dist = std::sqrt(std::pow(current_pose_.pose.position.x - attack_target_.x, 2) + 
                           std::pow(current_pose_.pose.position.y - attack_target_.y, 2));
    if (dist < 1.0) {
      payload_manager_.release();
      RCLCPP_INFO(this->get_logger(), "IMPACT DETECTED! Mission Accomplished.");
      is_kamikaze_mode_ = false;
    }
    return;
  }

  // Follower logic: maintain offset from leader
  if (swarm_members_.count(current_formation_.leader_id)) {
    auto leader_pose = swarm_members_[current_formation_.leader_id].current_pose;
    
    geometry_msgs::msg::PoseStamped cmd;
    cmd.header.stamp = this->now();
    cmd.header.frame_id = "map";
    
    // Simple offset based on UAV ID
    float offset_x = (uav_id_ % 2 == 0) ? 5.0 : -5.0;
    float offset_y = (uav_id_ > 2) ? 5.0 : -5.0;

    float target_x = leader_pose.pose.position.x + offset_x;
    float target_y = leader_pose.pose.position.y + offset_y;

    // --- VFH+ Obstacle Avoidance Integration ---
    float current_x = current_pose_.pose.position.x;
    float current_y = current_pose_.pose.position.y;
    
    // Calculate heading towards target
    float target_heading = std::atan2(target_y - current_y, target_x - current_x);
    if (target_heading < 0) target_heading += 2.0 * M_PI;
    
    if (!latest_obstacles_.empty()) {
      float increment = 2.0 * M_PI / latest_obstacles_.size();
      float safe_heading = vfh_planner_.compute_safe_heading(target_heading, latest_obstacles_, increment);
      
      // Move towards safe heading
      float speed = 1.5; 
      cmd.pose.position.x = current_x + speed * std::cos(safe_heading);
      cmd.pose.position.y = current_y + speed * std::sin(safe_heading);
    } else {
      cmd.pose.position.x = target_x;
      cmd.pose.position.y = target_y;
    }
    // -------------------------------------------

    cmd.pose.position.z = leader_pose.pose.position.z;

    cmd_pose_pub_->publish(cmd);
  }
}

void UavNode::select_leader()
{
  uint32_t potential_leader_id = uav_id_;
  float max_battery = battery_level_;

  for (const auto & member : swarm_members_) {
    if (member.second.battery_percentage > max_battery) {
      max_battery = member.second.battery_percentage;
      potential_leader_id = member.first;
    } else if (member.second.battery_percentage == max_battery) {
      // Tie-breaker: lowest ID
      if (member.first < potential_leader_id) {
        potential_leader_id = member.first;
      }
    }
  }

  bool new_leader_status = (potential_leader_id == uav_id_);
  if (new_leader_status != is_leader_) {
    is_leader_ = new_leader_status;
    RCLCPP_INFO(this->get_logger(), "Leader status changed: %s", is_leader_ ? "LEADER" : "FOLLOWER");
  }
}

} // namespace dug_core

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<dug_core::UavNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
