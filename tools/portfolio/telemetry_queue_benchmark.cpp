#include "system/event_queue.hpp"
#include "system/metrics.hpp"
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>

using netsim::system::MetricEvent;
using netsim::system::SpscRingBuffer;

int main() {
    constexpr uint32_t total_events = 1'000'000;
    SpscRingBuffer<MetricEvent, 1024> queue;
    MetricEvent event{
        0,
        "RouterA",
        "RouterB",
        1000,
        150,
        false
    };

    uint32_t produced = 0;
    uint32_t consumed = 0;

    const auto start = std::chrono::steady_clock::now();
    while (consumed < total_events) {
        if (produced < total_events && queue.push(event)) {
            ++produced;
        }

        while (queue.pop()) {
            ++consumed;
        }
    }
    const auto end = std::chrono::steady_clock::now();

    const double elapsed_seconds = std::chrono::duration<double>(end - start).count();
    const double events_per_second = static_cast<double>(total_events) / elapsed_seconds;

    std::cout << "Telemetry queue benchmark\n";
    std::cout << "Events processed: " << total_events << "\n";
    std::cout << "Elapsed seconds: " << std::fixed << std::setprecision(6) << elapsed_seconds << "\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) << events_per_second << " events/sec\n";
    std::cout << "Queue capacity: 1024 MetricEvent entries\n";

    return 0;
}
