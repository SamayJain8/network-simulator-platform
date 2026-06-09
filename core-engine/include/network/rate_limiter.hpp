#pragma once
#include <chrono>
#include <atomic>
#include <algorithm>

namespace netsim::network {

class GcraRateLimiter {
public:
    /**
     * @brief Construct a lock-free Generic Cell Rate Algorithm (GCRA) Rate Limiter.
     * 
     * @param rate_per_second Sustained packet allowance rate (e.g. 10.0 packets/sec).
     * @param burst_capacity Maximum burst tolerance (equivalent to bucket capacity).
     */
    GcraRateLimiter(double rate_per_second, double burst_capacity) noexcept {
        // T (Emission Interval) in nanoseconds
        emission_interval_ns_ = static_cast<uint64_t>(1'000'000'000.0 / rate_per_second);
        // Tau (Tolerance) in nanoseconds = (burst_capacity - 1) * T
        tolerance_ns_ = static_cast<uint64_t>((burst_capacity - 1.0) * emission_interval_ns_);
        
        tat_ns_.store(0, std::memory_order_relaxed);
    }

    // Disable copy/move semantics to guarantee thread safety
    GcraRateLimiter(const GcraRateLimiter&) = delete;
    GcraRateLimiter& operator=(const GcraRateLimiter&) = delete;

    /**
     * @brief Evaluates if a packet is permitted under rate limits.
     * Lock-free and thread-safe.
     * 
     * @return true if allowed, false if rate-limited.
     */
    bool allow() noexcept {
        auto now = std::chrono::steady_clock::now();
        uint64_t t = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

        uint64_t current_tat = tat_ns_.load(std::memory_order_relaxed);
        uint64_t new_tat = 0;

        while (true) {
            // If the last arrival is far in the past (idle bucket), reset calculation from current time t
            uint64_t base_tat = std::max(t, current_tat);
            
            // Check if the packet arrives before our tolerance window (rate limited)
            if (current_tat > t && (current_tat - t) > tolerance_ns_) {
                return false;
            }

            new_tat = base_tat + emission_interval_ns_;

            // Compare-And-Swap (CAS) Loop:
            // Attempt to write new_tat if no other thread modified current_tat.
            // Release memory ordering ensures preceding evaluations are synchronized.
            if (tat_ns_.compare_exchange_weak(current_tat, new_tat, 
                                              std::memory_order_release, 
                                              std::memory_order_relaxed)) {
                return true; // CAS succeeded; packet successfully processed
            }
            // If CAS fails, current_tat is automatically loaded with the updated value, 
            // and the loop retries immediately.
        }
    }

private:
    uint64_t emission_interval_ns_;
    uint64_t tolerance_ns_;
    
    // Theoretical Arrival Time (TAT) in absolute nanoseconds
    std::atomic<uint64_t> tat_ns_;
};

} // namespace netsim::network