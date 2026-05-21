#include "core/packet.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

namespace netsim::core {

// Change the constructor definition to this exact signature to match the linker requirement
Packet::Packet(uint8_t msg_type, const std::vector<uint8_t>& data) {
    header.version = 1;
    header.type = msg_type;
    header.length = static_cast<uint32_t>(data.size());
    payload = data;
}

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> buffer(sizeof(PacketHeader) + payload.size());
    
    // Copy the header
    PacketHeader net_header = header;
    net_header.length = htonl(header.length); // Network Byte Order (Big-Endian)
    
    std::memcpy(buffer.data(), &net_header, sizeof(PacketHeader));
    // Copy the payload
    std::memcpy(buffer.data() + sizeof(PacketHeader), payload.data(), payload.size());
    
    return buffer;
}

Packet Packet::deserialize(const std::vector<uint8_t>& raw_data) {
    if (raw_data.size() < sizeof(PacketHeader)) {
        throw std::runtime_error("Buffer too small for header");
    }

    Packet pkt;
    std::memcpy(&pkt.header, raw_data.data(), sizeof(PacketHeader));
    pkt.header.length = ntohl(pkt.header.length);
    
    if (raw_data.size() < sizeof(PacketHeader) + pkt.header.length) {
        throw std::runtime_error("Payload truncated");
    }

    pkt.payload.assign(raw_data.begin() + sizeof(PacketHeader), 
                       raw_data.begin() + sizeof(PacketHeader) + pkt.header.length);
    
    return pkt;
}

} // namespace netsim::core