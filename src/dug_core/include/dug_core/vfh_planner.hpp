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
    if (distances.empty() || increment <= 0.0f) {
      return target_heading;
    }

    size_t num_sectors = distances.size();
    std::vector<bool> occupancy_grid(num_sectors);
    for (size_t i = 0; i < num_sectors; ++i) {
      occupancy_grid[i] = (distances[i] < threshold_);
    }

    // Minimum valley width (number of consecutive clear sectors) to allow the UAV to pass through safely
    size_t min_valley_width = 3;
    if (num_sectors < min_valley_width) {
      min_valley_width = 1;
    }

    // Check if target heading is already in a safe valley of sufficient width
    int target_idx = static_cast<int>(std::round(target_heading / increment)) % num_sectors;
    if (target_idx < 0) target_idx += num_sectors;

    bool target_safe = false;
    for (size_t offset = 0; offset < min_valley_width; ++offset) {
      bool window_clear = true;
      for (size_t step = 0; step < min_valley_width; ++step) {
        int idx = (target_idx - static_cast<int>(offset) + static_cast<int>(step) + static_cast<int>(num_sectors)) % num_sectors;
        if (occupancy_grid[idx]) {
          window_clear = false;
          break;
        }
      }
      if (window_clear) {
        target_safe = true;
        break;
      }
    }

    if (target_safe) {
      return target_heading;
    }

    // Otherwise, search for the nearest safe valley center
    int best_idx = -1;
    size_t min_dist = num_sectors;

    for (size_t i = 0; i < num_sectors; ++i) {
      bool valley_clear = true;
      for (size_t step = 0; step < min_valley_width; ++step) {
        int idx = (i + step) % num_sectors;
        if (occupancy_grid[idx]) {
          valley_clear = false;
          break;
        }
      }
      if (valley_clear) {
        int valley_center_idx = (i + min_valley_width / 2) % num_sectors;
        
        int diff = std::abs(valley_center_idx - target_idx);
        if (diff > static_cast<int>(num_sectors / 2)) {
          diff = num_sectors - diff;
        }
        if (static_cast<size_t>(diff) < min_dist) {
          min_dist = diff;
          best_idx = valley_center_idx;
        }
      }
    }

    if (best_idx != -1) {
      return best_idx * increment;
    }

    return target_heading; // Fallback
  }

private:
  float threshold_;
};

} // namespace dug_core

#endif // DUG_CORE__VFH_PLANNER_HPP_
