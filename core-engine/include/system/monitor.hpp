#pragma once
#include "system/event_queue.hpp"
#include "system/metrics.hpp"
#include <thread>
#include <atomic>
#include <mutex>

namespace netsim::system {

struct AggregatedMetrics {
    double average_latency_ns;
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t dropped_packets;
    double throughput_kbps;
};

class TelemetryMonitor {
public:
    explicit TelemetryMonitor(SpscRingBuffer<MetricEvent, 1024>& queue) noexcept;
    ~TelemetryMonitor();

    // Prevent copying or movement
    TelemetryMonitor(const TelemetryMonitor&) = delete;
    TelemetryMonitor& operator=(const TelemetryMonitor&) = delete;

    void start();
    void stop() noexcept;

    [[nodiscard]] AggregatedMetrics get_latest_metrics() noexcept;

private:
    void monitor_loop();

    SpscRingBuffer<MetricEvent, 1024>& queue_;
    std::thread worker_thread_;
    std::atomic<bool> running_;

    std::mutex stats_mutex_;
    AggregatedMetrics stats_;
    
    // Internal counters
    uint64_t cumulative_latency_ns_ = 0;
    uint64_t cumulative_packets_ = 0;
    uint64_t cumulative_bytes_ = 0;
    uint64_t cumulative_dropped_ = 0;
};

} // namespace netsim::system