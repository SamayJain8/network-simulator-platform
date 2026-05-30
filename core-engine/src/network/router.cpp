#include "network/router.hpp"
#include <queue>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace netsim::network {

Router::Router(std::string name, IPv4Address ip, SubnetMask mask, std::string mac)
    : Node(std::move(name), ip, mask, std::move(mac)) {}

void Router::compute_routes(
    const std::unordered_map<std::string, std::vector<std::pair<std::string, uint32_t>>>& topology_graph,
    const std::unordered_map<std::string, IPv4Address>& node_to_ip,
    const std::unordered_map<std::string, SubnetMask>& node_to_mask) 
{
    std::string source = this->name();
    
    // Min-heap for Dijkstra: tracks pairs of (cumulative_cost, node_name)
    using PathNode = std::pair<uint32_t, std::string>;
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> min_heap;

    // Tracking maps
    std::unordered_map<std::string, uint32_t> distances;
    std::unordered_map<std::string, std::string> parent;

    // Initialize all distances to infinity
    for (const auto& [node, _] : topology_graph) {
        distances[node] = std::numeric_limits<uint32_t>::max();
    }

    // Safety check: ensure our source exists in the network graph
    if (distances.find(source) == distances.end()) {
        return; // Source is isolated or not in graph
    }

    distances[source] = 0;
    min_heap.push({0, source});

    while (!min_heap.empty()) {
        auto [current_dist, u] = min_heap.top();
        min_heap.pop();

        if (current_dist > distances[u]) {
            continue;
        }

        auto it = topology_graph.find(u);
        if (it == topology_graph.end()) {
            continue;
        }

        // Relax neighbors
        for (const auto& [v, link_cost] : it->second) {
            uint32_t new_dist = current_dist + link_cost;
            if (new_dist < distances[v]) {
                distances[v] = new_dist;
                parent[v] = u;
                min_heap.push({new_dist, v});
            }
        }
    }

    // Clear old routing table entries
    routing_table_.clear();

    // Reconstruct shortest paths to build routing table
    for (const auto& [dest, total_cost] : distances) {
        if (dest == source || total_cost == std::numeric_limits<uint32_t>::max()) {
            continue; // Skip self or unreachable nodes
        }

        // Backtrack path to find the immediate next hop next to the source
        std::string current = dest;
        while (parent.find(current) != parent.end() && parent[current] != source) {
            current = parent[current];
        }

        // Look up IP and Subnet details
        auto ip_it = node_to_ip.find(current);
        auto dest_ip_it = node_to_ip.find(dest);
        auto dest_mask_it = node_to_mask.find(dest);

        if (ip_it == node_to_ip.end() || dest_ip_it == node_to_ip.end() || dest_mask_it == node_to_mask.end()) {
            continue; // Incomplete metadata
        }

        IPv4Address next_hop_ip = ip_it->second;
        IPv4Address dest_ip = dest_ip_it->second;
        SubnetMask dest_mask = dest_mask_it->second;

        // Calculate network prefix for destination entry
        uint32_t subnet_raw = dest_ip.to_u32() & dest_mask.to_u32();

        RoutingTableEntry entry{
            IPv4Address(subnet_raw),
            dest_mask,
            next_hop_ip,
            "eth0",
            total_cost
        };
        routing_table_.push_back(entry);
    }
}

std::pair<bool, IPv4Address> Router::lookup_route(const IPv4Address& destination_ip) const noexcept {
    uint32_t dest_raw = destination_ip.to_u32();
    bool route_found = false;
    IPv4Address best_next_hop;
    uint8_t longest_prefix = 0;

    // Scan table evaluating standard Longest Prefix Match (LPM) boundaries
    for (const auto& entry : routing_table_) {
        uint32_t mask_val = entry.mask.to_u32();
        if ((dest_raw & mask_val) == (entry.destination_subnet.to_u32() & mask_val)) {
            uint8_t current_prefix = entry.mask.to_prefix_length();
            if (!route_found || current_prefix > longest_prefix) {
                route_found = true;
                best_next_hop = entry.next_hop;
                longest_prefix = current_prefix;
            }
        }
    }

    return {route_found, best_next_hop};
}

} // namespace netsim::network