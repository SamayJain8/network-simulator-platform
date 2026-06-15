#pragma once
#include "network/addressing.hpp"
#include "network/node.hpp"
#include "network/circuit_breaker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstdint>

namespace netsim::network {

struct RoutingTableEntry {
    IPv4Address destination_subnet;
    SubnetMask mask;
    uint32_t next_hop_id; // Optimized: Store ID instead of string name
    std::string interface_name;
    uint32_t metric;
};

class Router : public Node {
public:
    Router(uint32_t id, std::string name, IPv4Address ip, SubnetMask mask, std::string mac);

    /**
     * @brief Cache-friendly, zero-heap-allocation Dijkstra pathfinder.
     */
    void compute_routes(
        const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>>& topology_graph,
        const std::unordered_map<uint32_t, IPv4Address>& node_to_ip,
        const std::unordered_map<uint32_t, SubnetMask>& node_to_mask
    );

    [[nodiscard]] std::pair<bool, uint32_t> lookup_route(const IPv4Address& destination_ip) const noexcept;

    void record_link_success(uint32_t neighbor_id) noexcept;
    void record_link_failure(uint32_t neighbor_id) noexcept;
    
    [[nodiscard]] CircuitState get_link_state(uint32_t neighbor_id) noexcept;
    [[nodiscard]] const std::vector<RoutingTableEntry>& routing_table() const noexcept { return routing_table_; }

private:
    std::vector<RoutingTableEntry> routing_table_;
    std::unordered_map<uint32_t, CircuitBreaker> link_breakers_; // Optimized: key is uint32_t neighbor ID
};

} // namespace netsim::network