#include "core/message.hpp"
#include <utility>

namespace netsim::core {

Message::Message(std::string payload) noexcept : payload_(std::move(payload)) {}

std::vector<uint8_t> Message::serialize() const {
    return std::vector<uint8_t>(payload_.begin(), payload_.end());
}

Message Message::deserialize(const std::vector<uint8_t>& bytes) {
    return Message(std::string(bytes.begin(), bytes.end()));
}

} // namespace netsim::core