#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace netsim::core {

// RAII wrapper for the raw OS socket file descriptor
class SocketFD {
public:
    explicit SocketFD(int fd = -1) noexcept : fd_(fd) {}
    
    // Destructor ensures the OS resource is always released
    ~SocketFD() { close_socket(); }

    // Rule of 5: Disable copying to prevent double-closing the same socket
    SocketFD(const SocketFD&) = delete;
    SocketFD& operator=(const SocketFD&) = delete;

    // Enable move semantics (transferring ownership)
    SocketFD(SocketFD&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }
    SocketFD& operator=(SocketFD&& other) noexcept {
        if (this != &other) {
            close_socket();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool is_valid() const noexcept { return fd_ != -1; }

private:
    int fd_;
    void close_socket() noexcept {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }
};

// High-level Connection Abstraction and
class Connection {
public:
    // Factory method to establish a TCP connection
    static std::unique_ptr<Connection>connect_to(const std::string& ip, uint16_t port);
    
    // Constructor takes ownership of an active socket
    explicit Connection(SocketFD&& socket);

    // Core I/O operations
    ssize_t send_data(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_data(size_t max_bytes = 4096);

    void disconnect() noexcept;

private:
    SocketFD socket_;
    std::string remote_ip_;
    uint16_t remote_port_;
};

} // namespace netsim::core