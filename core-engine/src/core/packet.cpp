#include "core/packet.hpp"
#include <arpa/inet.h> // POSIX requirement for ntohl/htonl
#include <cstring>
#include <stdexcept>

namespace netsim::core {

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> buffer;
    // Total Header Size = 1 (ver) + 1 (type) + 4 (length) = 6 bytes
    buffer.reserve(6 + payload.size());

    buffer.push_back(version);
    buffer.push_back(static_cast<uint8_t>(type));

    // Convert length to Network Byte Order (Big-Endian)
    uint32_t net_len = htonl(static_cast<uint32_t>(payload.size()));
    
    // Copy the 4 bytes of the converted length into the buffer
    uint8_t len_bytes[4];
    std::memcpy(len_bytes, &net_len, 4);
    for(int i = 0; i < 4; ++i) buffer.push_back(len_bytes[i]);

    // Attach the actual payload
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    return buffer;
}

Packet Packet::deserialize(const std::vector<uint8_t>& raw_data) {
    // 1. Minimum Size Check: A valid packet must at least have a 6-byte header
    if (raw_data.size() < 6) {
        throw std::runtime_error("Incoming data too small to contain a valid header");
    }

    Packet packet;
    packet.version = raw_data[0];
    packet.type = static_cast<PacketType>(raw_data[1]);

    // 2. Extract length (Bytes 2, 3, 4, 5)
    uint32_t net_len;
    std::memcpy(&net_len, &raw_data[2], 4);
    
    // 3. THE FIX: Convert from Network Order (Big) to Host Order (Little)
    uint32_t host_len = ntohl(net_len);

    // 4. Integrity Check: Does the total size match the header's claim??
    if (raw_data.size() < 6 + host_len) {
        throw std::runtime_error("Packet payload size mismatch: corrupted stream");
    }

    // 5. Zero-Copy assignment: Extract payload
    packet.payload.assign(raw_data.begin() + 6, raw_data.begin() + 6 + host_len);
    
    return packet;
}

} // namespace netsim::core