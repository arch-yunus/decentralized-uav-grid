#ifndef DUG_CORE__VFH_PLANNER_HPP_
#define DUG_CORE__VFH_PLANNER_HPP_

#include <vector>
#include <cmath>
#include <algorithm>
#include <rclcpp/rclcpp.hpp>

namespace dug_core
{

class VfhPlanner
{
public:
  VfhPlanner() : threshold_(1.5) {}

  // Simplified VFH+ logic
  float compute_safe_heading(float target_heading, const std::vector<float>& distances, float increment)
  {
    // distances: 360 degree radial distances from sensor
    // threshold: distance below which an obstacle is considered dangerous
    
    std::vector<bool> occupancy_grid;
    for (float d : distances) {
      occupancy_grid.push_back(d < threshold_);
    }

    // If target heading is clear, return it
    int target_idx = static_cast<int>(target_heading / increment) % distances.size();
    if (!occupancy_grid[target_idx]) {
      return target_heading;
    }

    // Otherwise, search for the nearest clear sector
    for (size_t i = 1; i < distances.size() / 2; ++i) {
      int left_idx = (target_idx - i + distances.size()) % distances.size();
      int right_idx = (target_idx + i) % distances.size();

      if (!occupancy_grid[left_idx]) return left_idx * increment;
      if (!occupancy_grid[right_idx]) return right_idx * increment;
    }

    return target_heading; // Fallback
  }

private:
  float threshold_;
};

} // namespace dug_core

#endif // DUG_CORE__VFH_PLANNER_HPP_
