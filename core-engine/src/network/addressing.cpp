#include "network/addressing.hpp"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace netsim::network {

IPv4Address::IPv4Address(std::string_view ip_str) {
    uint32_t result = 0;
    int octets_found = 0;
    uint32_t current_octet = 0;
    bool reading_digit = false;

    for (char ch : ip_str) {
        if (ch >= '0' && ch <= '9') {
            current_octet = current_octet * 10 + (ch - '0');
            if (current_octet > 255) {
                throw std::invalid_argument("IPv4 octet value exceeds 255 limit");
            }
            reading_digit = true;
        } else if (ch == '.') {
            if (!reading_digit || octets_found >= 3) {
                throw std::invalid_argument("Malformed IPv4 segment separators");
            }
            result = (result << 8) | current_octet;
            current_octet = 0;
            octets_found++;
            reading_digit = false;
        } else {
            throw std::invalid_argument("Invalid symbol character in IPv4 address");
        }
    }
    
    if (octets_found != 3 || !reading_digit) {
        throw std::invalid_argument("Malformed IPv4 input segment count");
    }
    
    result = (result << 8) | current_octet;
    address_ = result;
}

std::string IPv4Address::to_string() const {
    return std::to_string((address_ >> 24) & 0xFF) + "." +
           std::to_string((address_ >> 16) & 0xFF) + "." +
           std::to_string((address_ >> 8) & 0xFF) + "." +
           std::to_string(address_ & 0xFF);
}

SubnetMask::SubnetMask(int prefix_length) {
    if (prefix_length < 0 || prefix_length > 32) {
        throw std::invalid_argument("Subnet prefix value bounds cannot exceed 32");
    }
    mask_ = (prefix_length == 0) ? 0 : (~0u << (32 - prefix_length));
}

SubnetMask::SubnetMask(std::string_view mask_str) {
    IPv4Address parsed_ip(mask_str);
    mask_ = parsed_ip.to_u32();
    
    // Validate that the mask contains contiguous 1s followed by contiguous 0s
    uint32_t m = mask_;
    if (m != 0) {
        uint32_t inverted = ~m;
        // Inverted check: contiguous 1s starting from trailing edge satisfies (X & (X + 1)) == 0
        if ((inverted & (inverted + 1)) != 0) {
            throw std::invalid_argument("Invalid subnet configuration: non-contiguous bits detected");
        }
    }
}

uint8_t SubnetMask::to_prefix_length() const noexcept {
    uint32_t m = mask_;
    uint8_t count = 0;
    while (m) {
        count += (m & 1);
        m >>= 1;
    }
    return count;
}

std::string SubnetMask::to_string() const {
    return IPv4Address(mask_).to_string();
}

} // namespace netsim::network