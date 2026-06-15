#include "network/router.hpp"
#include <queue>
#include <limits>
#include <algorithm>

namespace netsim::network {

Router::Router(uint32_t id, std::string name, IPv4Address ip, SubnetMask mask, std::string mac)
    : Node(id, std::move(name), ip, mask, std::move(mac)) {}

void Router::record_link_success(uint32_t neighbor_id) noexcept {
    link_breakers_[neighbor_id].record_success();
}

void Router::record_link_failure(uint32_t neighbor_id) noexcept {
    link_breakers_[neighbor_id].record_failure();
}

CircuitState Router::get_link_state(uint32_t neighbor_id) noexcept {
    return link_breakers_[neighbor_id].get_state();
}

void Router::compute_routes(
    const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>>& topology_graph,
    const std::unordered_map<uint32_t, IPv4Address>& node_to_ip,
    const std::unordered_map<uint32_t, SubnetMask>& node_to_mask) 
{
    // CRITICAL FIX: Clear the routing table at the start of recalculations to prevent infinite memory leaks
    routing_table_.clear();

    uint32_t source = this->id();
    
    // Cache-friendly: pair is {cost, node_id} using native uint32_t values.
    // Bypasses std::string heap allocations entirely on the hot path.
    using PathNode = std::pair<uint32_t, uint32_t>;
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> min_heap;

    std::unordered_map<uint32_t, uint32_t> distances;
    std::unordered_map<uint32_t, uint32_t> parent;

    for (const auto& [node, _] : topology_graph) {
        distances[node] = std::numeric_limits<uint32_t>::max();
    }

    if (distances.find(source) == distances.end()) return;

    distances[source] = 0;
    min_heap.push({0, source});

    while (!min_heap.empty()) {
        auto [current_dist, u] = min_heap.top();
        min_heap.pop();

        if (current_dist > distances[u]) continue;

        auto it = topology_graph.find(u);
        if (it == topology_graph.end()) continue;

        for (const auto& [v, original_cost] : it->second) {
            uint32_t cost = original_cost;

            if (u == source) {
                auto breaker_it = link_breakers_.find(v);
                if (breaker_it != link_breakers_.end()) {
                    if (breaker_it->second.get_state() == CircuitState::Open) {
                        cost = 999999; // Inflate cost of open circuit link
                    }
                }
            }

            uint32_t new_dist = current_dist + cost;
            if (new_dist < distances[v]) {
                distances[v] = new_dist;
                parent[v] = u;
                min_heap.push({new_dist, v});
            }
        }
    }

    for (const auto& [dest, total_cost] : distances) {
        if (dest == source || total_cost >= 999999) continue;

        uint32_t current = dest;
        while (parent.find(current) != parent.end() && parent[current] != source) {
            current = parent[current];
        }

        auto ip_it = node_to_ip.find(current);
        auto dest_ip_it = node_to_ip.find(dest);
        auto dest_mask_it = node_to_mask.find(dest);

        if (ip_it == node_to_ip.end() || dest_ip_it == node_to_ip.end() || dest_mask_it == node_to_mask.end()) {
            continue;
        }

        IPv4Address next_hop_ip = ip_it->second;
        IPv4Address dest_ip = dest_ip_it->second;
        SubnetMask dest_mask = dest_mask_it->second;

        uint32_t subnet_raw = dest_ip.to_u32() & dest_mask.to_u32();

        RoutingTableEntry entry{
            IPv4Address(subnet_raw),
            dest_mask,
            current, // Store next hop ID
            "eth0",
            total_cost
        };
        routing_table_.push_back(entry);
    }
}

std::pair<bool, uint32_t> Router::lookup_route(const IPv4Address& destination_ip) const noexcept {
    uint32_t dest_raw = destination_ip.to_u32();
    bool route_found = false;
    uint32_t best_next_hop = 0;
    uint8_t longest_prefix = 0;

    for (const auto& entry : routing_table_) {
        uint32_t mask_val = entry.mask.to_u32();
        if ((dest_raw & mask_val) == (entry.destination_subnet.to_u32() & mask_val)) {
            uint8_t current_prefix = entry.mask.to_prefix_length();
            if (!route_found || current_prefix > longest_prefix) {
                route_found = true;
                best_next_hop = entry.next_hop_id;
                longest_prefix = current_prefix;
            }
        }
    }

    return {route_found, best_next_hop};
}

} // namespace netsim::network