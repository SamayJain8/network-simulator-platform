#pragma once
#include "network/addressing.hpp"
#include <string>

namespace netsim::network {

class Node {
public:
    Node(std::string name, IPv4Address ip, SubnetMask mask, std::string mac);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const IPv4Address& ip_address() const noexcept { return ip_; }
    [[nodiscard]] const SubnetMask& subnet_mask() const noexcept { return mask_; }
    [[nodiscard]] const std::string& mac_address() const noexcept { return mac_; }

    // Evaluates if a target IP address is within the local subnet of this node
    [[nodiscard]] bool is_local(const IPv4Address& target_ip) const noexcept;

private:
    std::string name_;
    IPv4Address ip_;
    SubnetMask mask_;
    std::string mac_;
};

} // namespace netsim::network