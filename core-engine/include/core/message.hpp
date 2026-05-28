#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace netsim::core {

class Message {
public:
    Message() noexcept = default;
    explicit Message(std::string payload) noexcept;

    [[nodiscard]] const std::string& payload() const noexcept { return payload_; }
    
    // Serializes the logical message to raw bytes
    [[nodiscard]] std::vector<uint8_t> serialize() const;

    // Deserializes a raw byte stream back into a logical Message
    static Message deserialize(const std::vector<uint8_t>& bytes);

private:
    std::string payload_;
};

} // namespace netsim::core