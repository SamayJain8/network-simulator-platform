#include "core/packet.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

namespace netsim::core {

Packet::Packet() noexcept {
    std::memset(this, 0, sizeof(Packet));
}

Packet::Packet(uint8_t msg_type, const uint8_t* data, uint32_t len) noexcept {
    std::memset(this, 0, sizeof(Packet));
    header.version = 1;
    header.type = msg_type;
    header.length = (len > 1500) ? 1500 : len;
    if (data && header.length > 0) {
        std::memcpy(payload, data, header.length);
    }
}

Packet::Packet(uint8_t msg_type, const std::vector<uint8_t>& data) noexcept {
    std::memset(this, 0, sizeof(Packet));
    header.version = 1;
    header.type = msg_type;
    header.length = (data.size() > 1500) ? 1500 : data.size();
    if (!data.empty() && header.length > 0) {
        std::memcpy(payload, data.data(), header.length);
    }
}

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> buffer(sizeof(PacketHeader) + header.length);
    
    // Convert to Network Byte Order (Big-Endian)
    PacketHeader net_header = header;
    net_header.length = htonl(header.length);
    
    std::memcpy(buffer.data(), &net_header, sizeof(PacketHeader));
    if (header.length > 0) {
        std::memcpy(buffer.data() + sizeof(PacketHeader), payload, header.length);
    }
    
    return buffer;
}

Packet Packet::deserialize(const std::vector<uint8_t>& raw_data) {
    if (raw_data.size() < sizeof(PacketHeader)) {
        throw std::runtime_error("Buffer too small for header");
    }

    Packet pkt;
    std::memcpy(&pkt.header, raw_data.data(), sizeof(PacketHeader));
    pkt.header.length = ntohl(pkt.header.length);
    
    if (pkt.header.length > 1500) {
        throw std::runtime_error("Parsed payload length exceeds MTU limit");
    }

    if (raw_data.size() < sizeof(PacketHeader) + pkt.header.length) {
        throw std::runtime_error("Payload truncated");
    }

    if (pkt.header.length > 0) {
        std::memcpy(pkt.payload, raw_data.data() + sizeof(PacketHeader), pkt.header.length);
    }
    
    return pkt;
}

} // namespace netsim::core