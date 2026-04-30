#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <cstdint>

/**
 * @brief Represents the Data Link/Network Layer Packet.
 * This is the core unit of data moving through your simulator.
 */
struct Packet {
    // HEADER (5 Bytes total)
    uint32_t payload_size; // 4 Bytes: How much data is following?
    uint8_t type;          // 1 Byte: Is this DATA (0x01) or CONTROL (0x02)?

    // PAYLOAD (Variable)
    std::vector<uint8_t> payload;

    /**
     * @brief Transforms the struct into a 'Wire Format' (Binary)
     * Must handle Little-Endian (Mac M2) to Big-Endian (Network) conversion.
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Reconstructs a Packet from a raw stream of bytes.
     */
    static Packet deserialize(const std::vector<uint8_t>& buffer);
};

#endif // PACKET_HPP