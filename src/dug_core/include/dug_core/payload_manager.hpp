#ifndef DUG_CORE__PAYLOAD_MANAGER_HPP_
#define DUG_CORE__PAYLOAD_MANAGER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <string>

namespace dug_core
{

enum class PayloadState {
    SAFE,
    ARMED,
    RELEASED,
    DETONATED
};

class PayloadManager
{
public:
    PayloadManager() : state_(PayloadState::SAFE), munition_count_(2) {}

    void arm() {
        if (munition_count_ > 0) {
            state_ = PayloadState::ARMED;
            RCLCPP_WARN(rclcpp::get_logger("payload_manager"), "PAYLOAD ARMED - CAUTION!");
        }
    }

    void release() {
        if (state_ == PayloadState::ARMED) {
            state_ = PayloadState::RELEASED;
            munition_count_--;
            RCLCPP_INFO(rclcpp::get_logger("payload_manager"), "PAYLOAD RELEASED! Remaining: %d", munition_count_);
        }
    }

    PayloadState get_state() const { return state_; }
    int get_munition_count() const { return munition_count_; }

private:
    PayloadState state_;
    int munition_count_;
};

} // namespace dug_core

#endif // DUG_CORE__PAYLOAD_MANAGER_HPP_
