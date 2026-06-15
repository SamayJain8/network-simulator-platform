#pragma once
#include <chrono>
#include <atomic>

namespace netsim::network {

enum class CircuitState {
    Closed,
    Open,
    HalfOpen
};

class CircuitBreaker {
public:
    CircuitBreaker() noexcept;
    CircuitBreaker(uint32_t failure_threshold, uint64_t recovery_timeout_ms) noexcept;

    // Rule of 5: Handle atomic variables safely (they are non-copyable)
    CircuitBreaker(const CircuitBreaker& other) noexcept;
    CircuitBreaker& operator=(const CircuitBreaker& other) noexcept;

    bool record_success() noexcept;
    bool record_failure() noexcept;
    CircuitState get_state() noexcept;

    [[nodiscard]] uint32_t failure_count() const noexcept { return failure_count_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint32_t success_count() const noexcept { return success_count_.load(std::memory_order_relaxed); }

private:
    CircuitState state_;
    uint32_t failure_threshold_;
    uint64_t recovery_timeout_ms_;
    
    // CRITICAL FIX: Enforce thread-safety under concurrent packet evaluations using atomic variables
    std::atomic<uint32_t> failure_count_;
    std::atomic<uint32_t> success_count_;
    std::chrono::steady_clock::time_point last_state_change_time_;

    void transition_to(CircuitState new_state) noexcept;
};

} // namespace netsim::network