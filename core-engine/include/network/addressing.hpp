#pragma once
#include <string>
#include <string_view>
#include <cstdint>

namespace netsim::network {

class IPv4Address {
public:
    IPv4Address() noexcept : address_(0) {}
    explicit IPv4Address(uint32_t raw_address) noexcept : address_(raw_address) {}
    explicit IPv4Address(std::string_view ip_str);

    [[nodiscard]] uint32_t to_u32() const noexcept { return address_; }
    [[nodiscard]] std::string to_string() const;

    bool operator==(const IPv4Address& other) const noexcept { return address_ == other.address_; }

private:
    uint32_t address_;
};

class SubnetMask {
public:
    SubnetMask() noexcept : mask_(0) {}
    explicit SubnetMask(uint32_t raw_mask) noexcept : mask_(raw_mask) {}
    explicit SubnetMask(int prefix_length); // e.g., 24 -> 255.255.255.0
    explicit SubnetMask(std::string_view mask_str);

    [[nodiscard]] uint32_t to_u32() const noexcept { return mask_; }
    [[nodiscard]] uint8_t to_prefix_length() const noexcept;
    [[nodiscard]] std::string to_string() const;

private:
    uint32_t mask_;
};

} // namespace netsim::network