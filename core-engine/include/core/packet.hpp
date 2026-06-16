#pragma once
#include <cstdint>
#include <vector>

namespace netsim::core {

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t version;    // 1 byte
    uint8_t type;       // 1 byte (0: Data, 1: ACK, 2: control)
    uint32_t length;    // 4 bytes (Size of payload)
};
#pragma pack(pop)

struct Packet {
    PacketHeader header;
    // Bounded to 1500 bytes (Standard Ethernet MTU). 
    // This makes the entire Packet struct Trivially Destructible, 
    // resolving the MemoryArena destructor leak.
    uint8_t payload[1500];

    Packet() noexcept;
    Packet(uint8_t msg_type, const uint8_t* data, uint32_t len) noexcept;
    Packet(uint8_t msg_type, const std::vector<uint8_t>& data) noexcept;

    // Flattens object to bytes for the socket
    [[nodiscard]] std::vector<uint8_t> serialize() const;
      
    // Rebuilds object from socket bytes
    static Packet deserialize(const std::vector<uint8_t>& raw_data);
};

} // namespace netsim::core