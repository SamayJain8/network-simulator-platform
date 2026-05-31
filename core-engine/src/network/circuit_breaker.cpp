#include "network/circuit_breaker.hpp"

namespace netsim::network {

CircuitBreaker::CircuitBreaker() noexcept
    : state_(CircuitState::Closed), failure_threshold_(3), recovery_timeout_ms_(5000),
      failure_count_(0), success_count_(0), last_state_change_time_(std::chrono::steady_clock::now()) {}

CircuitBreaker::CircuitBreaker(uint32_t failure_threshold, uint64_t recovery_timeout_ms) noexcept
    : state_(CircuitState::Closed), failure_threshold_(failure_threshold), recovery_timeout_ms_(recovery_timeout_ms),
      failure_count_(0), success_count_(0), last_state_change_time_(std::chrono::steady_clock::now()) {}

void CircuitBreaker::transition_to(CircuitState new_state) noexcept {
    state_ = new_state;
    failure_count_ = 0;
    success_count_ = 0;
    last_state_change_time_ = std::chrono::steady_clock::now();
}

CircuitState CircuitBreaker::get_state() noexcept {
    if (state_ == CircuitState::Open) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_state_change_time_).count();
        
        // Auto-transition to HALF-OPEN once the timeout expires
        if (elapsed >= static_cast<long long>(recovery_timeout_ms_)) {
            transition_to(CircuitState::HalfOpen);
        }
    }
    return state_;
}

bool CircuitBreaker::record_success() noexcept {
    get_state(); // Evaluate timeout transitions
    
    if (state_ == CircuitState::HalfOpen) {
        success_count_++;
        // Recover to CLOSED after 3 consecutive successful transmissions
        if (success_count_ >= 3) {
            transition_to(CircuitState::Closed);
            return true;
        }
    } else if (state_ == CircuitState::Closed) {
        failure_count_ = 0; // Reset consecutive failure counter
    }
    return false;
}

bool CircuitBreaker::record_failure() noexcept {
    get_state(); // Evaluate timeout transitions

    if (state_ == CircuitState::Closed) {
        failure_count_++;
        if (failure_count_ >= failure_threshold_) {
            transition_to(CircuitState::Open);
            return true; // Tripped to OPEN
        }
    } else if (state_ == CircuitState::HalfOpen) {
        // Any failure in HALF-OPEN transitions immediately back to OPEN
        transition_to(CircuitState::Open);
        return true;
    }
    return false;
}

} // namespace netsim::network