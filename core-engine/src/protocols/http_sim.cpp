#include "protocols/http_sim.hpp"
#include <algorithm>
#include <cctype>

namespace netsim::protocols {

HttpSim::HttpSim() noexcept {
    reset();
}

void HttpSim::reset() noexcept {
    state_ = HttpState::Method;
    request_ = HttpRequest{};
    current_token_.clear();
    current_key_.clear();
    content_length_ = 0;
    body_bytes_read_ = 0;
}

void HttpSim::transition_to(HttpState new_state) noexcept {
    state_ = new_state;
}

bool HttpSim::parse_chunk(std::span<const uint8_t> chunk) {
    for (const uint8_t byte : chunk) {
        const char ch = static_cast<char>(byte);

        switch (state_) {
            case HttpState::Method: {
                if (ch == ' ') {
                    request_.method = current_token_;
                    current_token_.clear();
                    transition_to(HttpState::Uri);
                } else {
                    current_token_ += ch;
                }
                break;
            }
            case HttpState::Uri: {
                if (ch == ' ') {
                    request_.uri = current_token_;
                    current_token_.clear();
                    transition_to(HttpState::Version);
                } else {
                    current_token_ += ch;
                }
                break;
            }
            case HttpState::Version: {
                if (ch == '\r') {
                    // Expect \n next
                } else if (ch == '\n') {
                    request_.version = current_token_;
                    current_token_.clear();
                    transition_to(HttpState::HeaderKey);
                } else {
                    current_token_ += ch;
                }
                break;
            }
            case HttpState::HeaderKey: {
                if (ch == '\r') {
                    current_token_.clear();
                    transition_to(HttpState::ExpectBodyLF);
                } else if (ch == ':') {
                    current_key_ = current_token_;
                    current_token_.clear();
                    transition_to(HttpState::HeaderValue);
                } else {
                    current_token_ += ch;
                }
                break;
            }
            case HttpState::HeaderValue: {
                if (ch == ' ' && current_token_.empty()) {
                    break; // Skip leading whitespace
                }
                if (ch == '\r') {
                    transition_to(HttpState::ExpectValueLF);
                } else {
                    current_token_ += ch;
                }
                break;
            }
            case HttpState::ExpectValueLF: {
                if (ch == '\n') {
                    request_.headers[current_key_] = current_token_;
                    
                    if (current_key_ == "Content-Length") {
                        try {
                            content_length_ = std::stoul(current_token_);
                        } catch (...) {
                            transition_to(HttpState::Error);
                            break;
                        }
                    }
                    current_token_.clear();
                    current_key_.clear();
                    transition_to(HttpState::HeaderKey);
                } else {
                    transition_to(HttpState::Error);
                }
                break;
            }
            case HttpState::ExpectBodyLF: {
                if (ch == '\n') {
                    if (content_length_ > 0) {
                        request_.body.reserve(content_length_);
                        transition_to(HttpState::Body);
                    } else {
                        transition_to(HttpState::Complete);
                    }
                } else {
                    transition_to(HttpState::Error);
                }
                break;
            }
            case HttpState::Body: {
                request_.body.push_back(byte);
                body_bytes_read_++;
                if (body_bytes_read_ >= content_length_) {
                    transition_to(HttpState::Complete);
                }
                break;
            }
            case HttpState::Complete:
            case HttpState::Error:
                break;
        }
    }

    return (state_ == HttpState::Complete);
}

} // namespace netsim::protocols