#pragma once
#include <chrono>
#include <cstdint>

namespace netsim::network {

enum class CircuitState {
    Closed,     // Normal operations
    Open,       // Link failing; all traffic diverted or dropped
    HalfOpen    // Probe phase; testing link recovery
};

class CircuitBreaker {
public:
    CircuitBreaker() noexcept;
    CircuitBreaker(uint32_t failure_threshold, uint64_t recovery_timeout_ms) noexcept;

    // Record a successful packet delivery. Returns true if state changed.
    bool record_success() noexcept;

    // Record a dropped/failed packet. Returns true if state changed.
    bool record_failure() noexcept;

    // Evaluates current state, automatically transitioning from OPEN to HALF-OPEN on timeout.
    CircuitState get_state() noexcept;

    [[nodiscard]] uint32_t failure_count() const noexcept { return failure_count_; }
    [[nodiscard]] uint32_t success_count() const noexcept { return success_count_; }

private:
    CircuitState state_;
    uint32_t failure_threshold_;
    uint64_t recovery_timeout_ms_;
    
    uint32_t failure_count_;
    uint32_t success_count_;
    std::chrono::steady_clock::time_point last_state_change_time_;

    void transition_to(CircuitState new_state) noexcept;
};

} // namespace netsim::network