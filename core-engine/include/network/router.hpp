#pragma once
#include "network/addressing.hpp"
#include "network/node.hpp"
#include "network/circuit_breaker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

namespace netsim::network {

struct RoutingTableEntry {
    IPv4Address destination_subnet;
    SubnetMask mask;
    IPv4Address next_hop;
    std::string interface_name;
    uint32_t metric;
};

class Router : public Node {
public:
    Router(std::string name, IPv4Address ip, SubnetMask mask, std::string mac);

    void compute_routes(
        const std::unordered_map<std::string, std::vector<std::pair<std::string, uint32_t>>>& topology_graph,
        const std::unordered_map<std::string, IPv4Address>& node_to_ip,
        const std::unordered_map<std::string, SubnetMask>& node_to_mask
    );

    [[nodiscard]] std::pair<bool, IPv4Address> lookup_route(const IPv4Address& destination_ip) const noexcept;

    // Link stability controls
    void record_link_success(const std::string& neighbor_node) noexcept;
    void record_link_failure(const std::string& neighbor_node) noexcept;
    
    [[nodiscard]] CircuitState get_link_state(const std::string& neighbor_node) noexcept;
    [[nodiscard]] const std::vector<RoutingTableEntry>& routing_table() const noexcept { return routing_table_; }

private:
    std::vector<RoutingTableEntry> routing_table_;
    
    // Tracks a CircuitBreaker for each outbound neighbor link
    std::unordered_map<std::string, CircuitBreaker> link_breakers_;
};

} // namespace netsim::network