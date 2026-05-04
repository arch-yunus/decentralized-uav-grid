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

  // Publishers
  swarm_pub_ = this->create_publisher<dug_msgs::msg::SwarmState>("/swarm/status", 10);

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
}

void UavNode::swarm_callback(const dug_msgs::msg::SwarmState::SharedPtr msg)
{
  if (msg->uav_id != uav_id_) {
    swarm_members_[msg->uav_id] = *msg;
  }
}

void UavNode::timer_callback()
{
  // Update and publish own status
  auto msg = dug_msgs::msg::SwarmState();
  msg.uav_id = uav_id_;
  msg.is_leader = is_leader_;
  msg.battery_percentage = battery_level_;
  msg.current_pose = current_pose_;
  
  swarm_pub_->publish(msg);

  // Perform dynamic leader selection
  select_leader();
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
