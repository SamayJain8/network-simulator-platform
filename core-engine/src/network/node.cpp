#include "network/node.hpp"
#include <utility>

namespace netsim::network {

Node::Node(uint32_t id, std::string name, IPv4Address ip, SubnetMask mask, std::string mac)
    : id_(id), name_(std::move(name)), ip_(ip), mask_(mask), mac_(std::move(mac)), rate_limiter_(nullptr) {}

bool Node::is_local(const IPv4Address& target_ip) const noexcept {
    uint32_t mask_val = mask_.to_u32();
    uint32_t local_subnet = ip_.to_u32() & mask_val;
    uint32_t target_subnet = target_ip.to_u32() & mask_val;
    return local_subnet == target_subnet;
}

void Node::enable_rate_limiting(double rate_per_sec, double burst_capacity) noexcept {
    rate_limiter_ = std::make_unique<GcraRateLimiter>(rate_per_sec, burst_capacity);
}

bool Node::receive_packet() noexcept {
    if (!rate_limiter_) {
        return true;
    }
    return rate_limiter_->allow();
}

} // namespace netsim::network