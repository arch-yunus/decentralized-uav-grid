#ifndef DUG_CORE__UAV_NODE_HPP_
#define DUG_CORE__UAV_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <mavros_msgs/msg/state.hpp>
#include <dug_msgs/msg/swarm_state.hpp>
#include <dug_msgs/msg/target_info.hpp>

namespace dug_core
{

class UavNode : public rclcpp::Node
{
public:
  UavNode();

private:
  // Callbacks
  void state_callback(const mavros_msgs::msg::State::SharedPtr msg);
  void pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg);
  void swarm_callback(const dug_msgs::msg::SwarmState::SharedPtr msg);
  void target_callback(const dug_msgs::msg::TargetInfo::SharedPtr msg);
  void formation_callback(const dug_msgs::msg::FormationState::SharedPtr msg);
  void timer_callback();

  // Helper functions
  void select_leader();
  void perform_formation_control();

  // Subscriptions
  rclcpp::Subscription<mavros_msgs::msg::State>::SharedPtr state_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr pose_sub_;
  rclcpp::Subscription<dug_msgs::msg::SwarmState>::SharedPtr swarm_sub_;
  rclcpp::Subscription<dug_msgs::msg::TargetInfo>::SharedPtr target_sub_;
  rclcpp::Subscription<dug_msgs::msg::FormationState>::SharedPtr formation_sub_;

  // Publishers
  rclcpp::Publisher<dug_msgs::msg::SwarmState>::SharedPtr swarm_pub_;
  rclcpp::Publisher<dug_msgs::msg::FormationState>::SharedPtr formation_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr cmd_pose_pub_;

  // Timers
  rclcpp::TimerBase::SharedPtr timer_;

  // Node data
  uint32_t uav_id_;
  bool is_leader_;
  float battery_level_;
  mavros_msgs::msg::State current_state_;
  geometry_msgs::msg::PoseStamped current_pose_;
  
  std::map<uint32_t, dug_msgs::msg::SwarmState> swarm_members_;
  std::vector<dug_msgs::msg::TargetInfo> global_targets_;
  dug_msgs::msg::FormationState current_formation_;
};

} // namespace dug_core

#endif // DUG_CORE__UAV_NODE_HPP_
