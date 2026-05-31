#pragma once
#include <cstdint>
#include <random>
#include <algorithm>

namespace netsim::network {

class Backoff {
public:
    Backoff(uint64_t base_ms, uint64_t max_ms) noexcept
        : base_ms_(base_ms), max_ms_(max_ms), generator_(std::random_device{}()) {}

    [[nodiscard]] uint64_t calculate_delay(uint32_t attempt) noexcept {
        if (attempt == 0) return 0;
        
        // Exponential calculation: base * 2^(attempt - 1)
        uint64_t exp_factor = 1u << (attempt - 1);
        uint64_t delay = base_ms_ * exp_factor;
        delay = std::min(delay, max_ms_);

        // Apply Full Jitter: randomize interval uniformly between 0 and calculated delay
        std::uniform_int_distribution<uint64_t> distribution(0, delay);
        return distribution(generator_);
    }

private:
    uint64_t base_ms_;
    uint64_t max_ms_;
    std::mt19937_64 generator_;
};

} // namespace netsim::network