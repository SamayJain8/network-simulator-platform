#include "network/circuit_breaker.hpp"

namespace netsim::network {

CircuitBreaker::CircuitBreaker() noexcept
    : state_(CircuitState::Closed), failure_threshold_(3), recovery_timeout_ms_(5000),
      failure_count_(0), success_count_(0), last_state_change_time_(std::chrono::steady_clock::now()) {}

CircuitBreaker::CircuitBreaker(uint32_t failure_threshold, uint64_t recovery_timeout_ms) noexcept
    : state_(CircuitState::Closed), failure_threshold_(failure_threshold), recovery_timeout_ms_(recovery_timeout_ms),
      failure_count_(0), success_count_(0), last_state_change_time_(std::chrono::steady_clock::now()) {}

// Handle thread-safe copy constructor on non-copyable std::atomic variables
CircuitBreaker::CircuitBreaker(const CircuitBreaker& other) noexcept
    : state_(other.state_), failure_threshold_(other.failure_threshold_), 
      recovery_timeout_ms_(other.recovery_timeout_ms_), 
      failure_count_(other.failure_count_.load(std::memory_order_relaxed)), 
      success_count_(other.success_count_.load(std::memory_order_relaxed)), 
      last_state_change_time_(other.last_state_change_time_) {}

CircuitBreaker& CircuitBreaker::operator=(const CircuitBreaker& other) noexcept {
    if (this != &other) {
        state_ = other.state_;
        failure_threshold_ = other.failure_threshold_;
        recovery_timeout_ms_ = other.recovery_timeout_ms_;
        failure_count_.store(other.failure_count_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        success_count_.store(other.success_count_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        last_state_change_time_ = other.last_state_change_time_;
    }
    return *this;
}

void CircuitBreaker::transition_to(CircuitState new_state) noexcept {
    state_ = new_state;
    failure_count_.store(0, std::memory_order_relaxed);
    success_count_.store(0, std::memory_order_relaxed);
    last_state_change_time_ = std::chrono::steady_clock::now();
}

CircuitState CircuitBreaker::get_state() noexcept {
    if (state_ == CircuitState::Open) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_state_change_time_).count();
        
        if (elapsed >= static_cast<long long>(recovery_timeout_ms_)) {
            transition_to(CircuitState::HalfOpen);
        }
    }
    return state_;
}

bool CircuitBreaker::record_success() noexcept {
    get_state();
    
    if (state_ == CircuitState::HalfOpen) {
        // Thread-safe atomic increment using relaxed memory orderings
        uint32_t prev_success = success_count_.fetch_add(1, std::memory_order_relaxed);
        if (prev_success + 1 >= 3) {
            transition_to(CircuitState::Closed);
            return true;
        }
    } else if (state_ == CircuitState::Closed) {
        failure_count_.store(0, std::memory_order_relaxed);
    }
    return false;
}

bool CircuitBreaker::record_failure() noexcept {
    get_state();

    if (state_ == CircuitState::Closed) {
        uint32_t prev_failures = failure_count_.fetch_add(1, std::memory_order_relaxed);
        if (prev_failures + 1 >= failure_threshold_) {
            transition_to(CircuitState::Open);
            return true;
        }
    } else if (state_ == CircuitState::HalfOpen) {
        transition_to(CircuitState::Open);
        return true;
    }
    return false;
}

} // namespace netsim::network