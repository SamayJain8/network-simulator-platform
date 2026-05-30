#pragma once
#include <cstdint>
#include <string>

namespace netsim::system {

struct MetricEvent {
    uint64_t timestamp_us;
    std::string source_node;
    std::string dest_node;
    uint32_t packet_size; // Bytes
    uint64_t latency_ns;
    bool dropped;
};

} // namespace netsim::system