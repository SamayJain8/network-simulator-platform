#ifndef NETWORK_SIM_PACKET_HPP
#define NETWORK_SIM_PACKET_HPP

#include <vector>
#include <cstdint>

namespace network_sim::protocols {

/**
 * @brief Represents a Data Packet in the simulator.
 * Located in: include/protocols/packet.hpp
 */
struct Packet {
    // Header (Fixed size)
    uint32_t payload_size; 
    uint8_t type;          

    // Data (Variable size)
    std::vector<uint8_t> payload;

    // Phase 2 logic to be implemented tomorrow
    std::vector<uint8_t> serialize() const;
    static Packet deserialize(const std::vector<uint8_t>& buffer);
};

} // namespace network_sim::protocols

#endif