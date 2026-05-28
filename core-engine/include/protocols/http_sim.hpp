#include "protocols/protocol.hpp"
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace netsim::protocols {

enum class HttpState {
    Method,
    Uri,
    Version,
    HeaderKey,
    HeaderValue,
    ExpectValueLF,
    ExpectBodyLF,
    Body,
    Complete,
    Error
};

struct HttpRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};

class HttpSim : public Protocol {
public:
    HttpSim() noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "HTTP/1.1"; }
    bool parse_chunk(std::span<const uint8_t> chunk) override;
    void reset() noexcept override;

    [[nodiscard]] const HttpRequest& get_request() const noexcept { return request_; }

private:
    HttpState state_;
    HttpRequest request_;
    
    std::string current_token_;
    std::string current_key_;
    size_t content_length_;
    size_t body_bytes_read_;

    void transition_to(HttpState new_state) noexcept;
};

} // namespace netsim::protocols
