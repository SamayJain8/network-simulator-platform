#include "system/monitor.hpp"
#include <chrono>

namespace netsim::system {

TelemetryMonitor::TelemetryMonitor(SpscRingBuffer<MetricEvent, 1024>& queue) noexcept
    : queue_(queue), running_(false), stats_{0.0, 0, 0, 0, 0.0} {}

TelemetryMonitor::~TelemetryMonitor() {
    stop();
}

void TelemetryMonitor::start() {
    if (running_.load(std::memory_order_relaxed)) return;
    running_.store(true, std::memory_order_relaxed);
    worker_thread_ = std::thread(&TelemetryMonitor::monitor_loop, this);
}

void TelemetryMonitor::stop() noexcept {
    if (!running_.load(std::memory_order_relaxed)) return;
    running_.store(false, std::memory_order_relaxed);
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

AggregatedMetrics TelemetryMonitor::get_latest_metrics() noexcept {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void TelemetryMonitor::monitor_loop() {
    auto last_time = std::chrono::steady_clock::now();

    while (running_.load(std::memory_order_relaxed)) {
        bool processed_any = false;

        // Drain the lock-free ring buffer
        while (auto event_opt = queue_.pop()) {
            processed_any = true;
            const auto& event = *event_opt;

            if (!event.dropped) {
                cumulative_latency_ns_ += event.latency_ns;
                cumulative_bytes_ += event.packet_size;
            } else {
                cumulative_dropped_++;
            }
            cumulative_packets_++;
        }

        // Calculate and update metrics every 100 milliseconds
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
        
        if (elapsed >= 100) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            
            stats_.total_packets = cumulative_packets_;
            stats_.dropped_packets = cumulative_dropped_;
            stats_.total_bytes += cumulative_bytes_; // Cumulative total
            
            // Calculate average latency
            uint64_t successful_packets = cumulative_packets_ - cumulative_dropped_;
            if (successful_packets > 0) {
                stats_.average_latency_ns = static_cast<double>(cumulative_latency_ns_) / successful_packets;
            } else {
                stats_.average_latency_ns = 0.0;
            }

            // Calculate rolling throughput (kilobits per second)
            double elapsed_sec = elapsed / 1000.0;
            if (elapsed_sec > 0.0) {
                stats_.throughput_kbps = (cumulative_bytes_ * 8.0) / (elapsed_sec * 1000.0);
            } else {
                stats_.throughput_kbps = 0.0;
            }

            // Reset interval counters for rolling throughput, but preserve totals
            cumulative_bytes_ = 0;
            last_time = now;
        }

        // Yield CPU thread when idle to prevent high CPU utilization
        if (!processed_any) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace netsim::system