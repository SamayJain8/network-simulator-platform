#pragma once
#include "system/metrics.hpp"
#include <atomic>
#include <array>
#include <optional>
#include <new>

namespace netsim::system {

template <typename T, size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity size must be a power of 2");
public:
    SpscRingBuffer() noexcept : head_(0), tail_(0) {}

    // Producer writes data (called strictly from the packet forwarding engine thread)
    // Returns false if queue is full
    bool push(const T& item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t current_head = head_.load(std::memory_order_acquire);

        if ((current_tail - current_head) >= Capacity) {
            return false; // Queue saturated (full)
        }

        buffer_[current_tail & (Capacity - 1)] = item;
        // Release ordering guarantees that buffer changes are visible before tail pointer updates
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    // Consumer reads data (called strictly from the background monitoring thread)
    // Returns std::nullopt if queue is empty
    std::optional<T> pop() noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return std::nullopt; // Queue depleted (empty)
        }

        T item = buffer_[current_head & (Capacity - 1)];
        // Release ordering guarantees that the item is fully copied out before the head pointer updates
        head_.store(current_head + 1, std::memory_order_release);
        return item;
    }

    [[nodiscard]] size_t size() const noexcept {
        size_t t = tail_.load(std::memory_order_relaxed);
        size_t h = head_.load(std::memory_order_relaxed);
        return (t >= h) ? (t - h) : 0;
    }

private:
    std::array<T, Capacity> buffer_;

    // Prevent False Sharing by aligning atomic pointers to distinct cache line boundaries (64 bytes)
    // This isolates head and tail pointers onto separate L1/L2 cache lines
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace netsim::system