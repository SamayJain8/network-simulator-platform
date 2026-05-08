#pragma once
#include <cstdint>
#include <vector>

namespace netsim::core {

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t version;    // 1 byte
    uint8_t type;       // 1 byte (0: Data, 1: ACK, 2: control)
    uint32_t length;    // 4 bytes (i's the Size of payload)
};
#pragma pack(pop)

class Packet {
public:
    PacketHeader header;
    std::vector<uint8_t> payload;


    Packet(uint8_t msg_type, const std::vector<uint8_t>& data);

    // Flattens object to bytes for the socket
    std::vector<uint8_t> serialize() const;
      
    // Rebuilds object from socket bytes
    static Packet deserialize(const std::vector<uint8_t>& raw_data);
};

} // namespace netsim::corea 