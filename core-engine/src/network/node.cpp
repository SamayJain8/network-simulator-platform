#include "network/node.hpp"
#include <utility>

namespace netsim::network {

Node::Node(std::string name, IPv4Address ip, SubnetMask mask, std::string mac)
    : name_(std::move(name)), ip_(ip), mask_(mask), mac_(std::move(mac)) {}

bool Node::is_local(const IPv4Address& target_ip) const noexcept {
    uint32_t mask_val = mask_.to_u32();
    uint32_t local_subnet = ip_.to_u32() & mask_val;
    uint32_t target_subnet = target_ip.to_u32() & mask_val;
    return local_subnet == target_subnet;
}

} // namespace netsim::network