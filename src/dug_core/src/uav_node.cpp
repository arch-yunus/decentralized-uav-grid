#include "dug_core/uav_node.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <algorithm>

namespace dug_core
{

class Sha256 {
private:
    static inline uint32_t rotate_right(uint32_t value, uint32_t count) {
        return (value >> count) | (value << (32 - count));
    }

public:
    static std::string hash(const std::string& input) {
        uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
        uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

        std::vector<uint8_t> msg(input.begin(), input.end());
        uint64_t bit_len = msg.size() * 8;
        msg.push_back(0x80);
        while ((msg.size() + 8) % 64 != 0) {
            msg.push_back(0x00);
        }
        for (int i = 7; i >= 0; --i) {
            msg.push_back((bit_len >> (i * 8)) & 0xff);
        }

        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (msg[chunk + i * 4] << 24) |
                       (msg[chunk + i * 4 + 1] << 16) |
                       (msg[chunk + i * 4 + 2] << 8) |
                       (msg[chunk + i * 4 + 3]);
            }
            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = (rotate_right(w[i - 15], 7) ^ rotate_right(w[i - 15], 18) ^ (w[i - 15] >> 3));
                uint32_t s1 = (rotate_right(w[i - 2], 17) ^ rotate_right(w[i - 2], 19) ^ (w[i - 2] >> 10));
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

            for (int i = 0; i < 64; ++i) {
                uint32_t S1 = (rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25));
                uint32_t ch = ((e & f) ^ (~e & g));
                uint32_t temp1 = h + S1 + ch + k[i] + w[i];
                uint32_t S0 = (rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22));
                uint32_t maj = ((a & b) ^ (a & c) ^ (b & c));
                uint32_t temp2 = S0 + maj;

                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            h0 += a; h1 += b; h2 += c; h3 += d;
            h4 += e; h5 += f; h6 += g; h7 += h;
        }

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        ss << std::setw(8) << h0 << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
           << std::setw(8) << h4 << std::setw(8) << h5 << std::setw(8) << h6 << std::setw(8) << h7;
        return ss.str();
    }
};

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
    "vslam_pose", 10, std::bind(&UavNode::vslam_callback, this, std::placeholders::_1));

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

std::string UavNode::compute_hash_signature(uint32_t uav_id)
{
  int64_t time_window = static_cast<int64_t>(std::time(nullptr)) / 10;
  std::string secret_key = "dug_secure_key_2026";
  std::string data = std::to_string(uav_id) + ":" + std::to_string(time_window) + ":" + secret_key;
  return Sha256::hash(data);
}

bool UavNode::verify_peer_signature(uint32_t uav_id, const std::string& signature)
{
  int64_t current_time = static_cast<int64_t>(std::time(nullptr));
  int64_t window0 = current_time / 10;
  int64_t window1 = window0 - 1;
  int64_t window2 = window0 + 1;

  std::string secret_key = "dug_secure_key_2026";
  
  std::string data0 = std::to_string(uav_id) + ":" + std::to_string(window0) + ":" + secret_key;
  if (Sha256::hash(data0) == signature) return true;

  std::string data1 = std::to_string(uav_id) + ":" + std::to_string(window1) + ":" + secret_key;
  if (Sha256::hash(data1) == signature) return true;

  std::string data2 = std::to_string(uav_id) + ":" + std::to_string(window2) + ":" + secret_key;
  if (Sha256::hash(data2) == signature) return true;

  return false;
}

void UavNode::swarm_callback(const dug_msgs::msg::SwarmState::SharedPtr msg)
{
  if (msg->uav_id != uav_id_) {
    if (verify_peer_signature(msg->uav_id, msg->signature)) {
      swarm_members_[msg->uav_id] = *msg;
      last_seen_peers_[msg->uav_id] = this->now();
    } else {
      RCLCPP_WARN(this->get_logger(), "Zero-Trust: Rejected message from UAV %d due to invalid signature!", msg->uav_id);
    }
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

  // Drain battery in simulation
  battery_level_ = std::max(0.0f, battery_level_ - 0.1f);

  // Calculate active peers (connectivity degree)
  uint32_t own_connectivity = 0;
  rclcpp::Time now_time = this->now();
  for (auto const& [id, last_seen] : last_seen_peers_) {
    if ((now_time - last_seen).seconds() < 5.0) {
      own_connectivity++;
    }
  }

  // Simulate hardware load
  float own_hw_load = 20.0f + 10.0f * std::sin(now_time.seconds() * 0.1f) + (uav_id_ * 5.0f);
  own_hw_load = std::max(0.0f, std::min(100.0f, own_hw_load));

  // Update and publish own status
  auto msg = dug_msgs::msg::SwarmState();
  msg.uav_id = uav_id_;
  msg.is_leader = is_leader_;
  msg.battery_percentage = battery_level_;
  msg.current_pose = current_pose_;
  msg.connectivity_degree = own_connectivity;
  msg.hardware_load = own_hw_load;
  msg.signature = compute_hash_signature(uav_id_);
  
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
  
  // Calculate our own active peers (connectivity degree)
  uint32_t own_connectivity = 0;
  rclcpp::Time now_time = this->now();
  for (auto const& [id, last_seen] : last_seen_peers_) {
    if ((now_time - last_seen).seconds() < 5.0) {
      own_connectivity++;
    }
  }
  
  float own_hw_load = 20.0f + 10.0f * std::sin(now_time.seconds() * 0.1f) + (uav_id_ * 5.0f);
  own_hw_load = std::max(0.0f, std::min(100.0f, own_hw_load));
  
  // Score formula: Score_i = w1 * B_i + w2 * C_i - w3 * L_i
  float max_score = 0.5f * battery_level_ + 3.0f * static_cast<float>(own_connectivity) - 0.2f * own_hw_load;

  for (const auto & member : swarm_members_) {
    // Check if member is active
    if (last_seen_peers_.count(member.first) && (now_time - last_seen_peers_[member.first]).seconds() < 5.0) {
      float m_score = 0.5f * member.second.battery_percentage + 
                      3.0f * static_cast<float>(member.second.connectivity_degree) - 
                      0.2f * member.second.hardware_load;
      if (m_score > max_score) {
        max_score = m_score;
        potential_leader_id = member.first;
      } else if (m_score == max_score) {
        // Tie-breaker: lowest ID
        if (member.first < potential_leader_id) {
          potential_leader_id = member.first;
        }
      }
    }
  }

  bool new_leader_status = (potential_leader_id == uav_id_);
  if (new_leader_status != is_leader_) {
    is_leader_ = new_leader_status;
    RCLCPP_INFO(this->get_logger(), "Leader status changed: %s (Max Score: %.2f)", is_leader_ ? "LEADER" : "FOLLOWER", max_score);
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
