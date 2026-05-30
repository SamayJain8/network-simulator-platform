#include "network/addressing.hpp"
#include "network/node.hpp"
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

    /**
     * @brief Dynamic Route Recalculation using Dijkstra's shortest path algorithm.
     * Mimics Link-State routing behavior (e.g., OSPF).
     * 
     * @param topology_graph Adjacency map of nodes and link costs: NodeName -> [(NeighborName, LinkCost)]
     * @param node_to_ip Map associating logical node names with physical interface IPs
     * @param node_to_mask Map associating logical node names with matching SubnetMask allocations
     */
    void compute_routes(
        const std::unordered_map<std::string, std::vector<std::pair<std::string, uint32_t>>>& topology_graph,
        const std::unordered_map<std::string, IPv4Address>& node_to_ip,
        const std::unordered_map<std::string, SubnetMask>& node_to_mask
    );

    /**
     * @brief Resolves next-hop address using Longest Prefix Match (LPM)
     * @return std::pair<bool route_found, IPv4Address next_hop_ip>
     */
    [[nodiscard]] std::pair<bool, IPv4Address> lookup_route(const IPv4Address& destination_ip) const noexcept;

    [[nodiscard]] const std::vector<RoutingTableEntry>& routing_table() const noexcept { return routing_table_; }

private:
    std::vector<RoutingTableEntry> routing_table_;
};

} // namespace netsim::network