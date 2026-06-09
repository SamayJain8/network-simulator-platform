#pragma once
#include "network/addressing.hpp"
#include "network/rate_limiter.hpp"
#include <string>
#include <memory>

namespace netsim::network {

class Node {
public:
    Node(std::string name, IPv4Address ip, SubnetMask mask, std::string mac);
    virtual ~Node() = default;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const IPv4Address& ip_address() const noexcept { return ip_; }
    [[nodiscard]] const SubnetMask& subnet_mask() const noexcept { return mask_; }
    [[nodiscard]] const std::string& mac_address() const noexcept { return mac_; }

    [[nodiscard]] bool is_local(const IPv4Address& target_ip) const noexcept;

    // Configure the lock-free GCRA limiter on this node
    void enable_rate_limiting(double rate_per_sec, double burst_capacity) noexcept;
    
    // Processes inbound packet events. Returns false if rate-limited.
    [[nodiscard]] bool receive_packet() noexcept;

private:
    std::string name_;
    IPv4Address ip_;
    SubnetMask mask_;
    std::string mac_;

    std::unique_ptr<GcraRateLimiter> rate_limiter_;
};

} // namespace netsim::network