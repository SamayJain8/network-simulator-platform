#pragma once
#include <span>
#include <cstdint>
#include <string_view>

namespace netsim::protocols {

class Protocol {
public:
    virtual ~Protocol() = default;
    
    // Returns the human-readable name of the application protocol
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    
    // Parses incoming raw data blocks. Returns true when the message boundary is complete.
    virtual bool parse_chunk(std::span<const uint8_t> chunk) = 0;
    
    // Resets internal parsing state registers
    virtual void reset() noexcept = 0;
};

} // namespace netsim::protocols