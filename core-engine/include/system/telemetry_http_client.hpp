#pragma once
#include "system/metrics.hpp"
#include <cstdint>
#include <string>

namespace netsim::system {

struct TelemetryEndpoint {
    std::string host;
    std::string port;
    std::string path;
};

struct TelemetrySample {
    uint64_t timestamp_us;
    std::string source_node;
    std::string dest_node;
    uint32_t packet_size;
    double latency_ns;
    uint32_t dropped_packets;
    double throughput_kbps;
};

class TelemetryHttpClient {
public:
    explicit TelemetryHttpClient(std::string endpoint_url);

    [[nodiscard]] bool post_sample(const TelemetrySample& sample) const noexcept;
    [[nodiscard]] bool post_metric_event(const MetricEvent& event, double throughput_kbps) const noexcept;
    [[nodiscard]] const std::string& endpoint_url() const noexcept { return endpoint_url_; }

private:
    std::string endpoint_url_;
    TelemetryEndpoint endpoint_;
};

} // namespace netsim::system
