#include "core/connection.hpp"
#include <arpa/inet.h>
#include <system_error>
#include <stdexcept>

namespace netsim::core {

Connection::Connection(SocketFD&& socket) : socket_(std::move(socket)) {}

std::unique_ptr<Connection> Connection::connect_to(const std::string& ip, uint16_t port) {
    // 1. Request a raw IPv4 TCP socket from the OS
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }

    // Immediately wrap it in our RAII class for safety
    SocketFD safe_socket(sock);
    
    // 2. Configure the destination address and port
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (::inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::invalid_argument("Invalid IP address format: " + ip);
    }

    // 3. Attempt the TCP Handshake
    if (::connect(safe_socket.get(), reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::system_error(errno, std::system_category(), "Connection failed to " + ip);
    }

    auto conn = std::make_unique<Connection>(std::move(safe_socket));
    conn->remote_ip_ = ip;
    conn->remote_port_ = port;
    return conn;
}

ssize_t Connection::send_data(const std::vector<uint8_t>& data) {
    if (!socket_.is_valid()) throw std::runtime_error("Socket is closed");

    ssize_t total_sent = 0;
    size_t left_to_send = data.size();
    
    // Robust sending loop: OS might not send all bytes in one go
    while (total_sent < static_cast<ssize_t>(data.size())) {
        ssize_t sent = ::send(socket_.get(), data.data() + total_sent, left_to_send, 0);
        if (sent < 0) {
            throw std::system_error(errno, std::system_category(), "Send failed");
        }
        total_sent += sent;
        left_to_send -= sent;
    }
    return total_sent;
}

std::vector<uint8_t> Connection::receive_data(size_t max_bytes) {
    if (!socket_.is_valid()) throw std::runtime_error("Socket is closed");

    std::vector<uint8_t> buffer(max_bytes);
    ssize_t bytes_read = ::recv(socket_.get(), buffer.data(), buffer.size(), 0);
    
    if (bytes_read < 0) {
        throw std::system_error(errno, std::system_category(), "Receive failed");
    } else if (bytes_read == 0) {
        disconnect(); // Peer cleanly closed the connection
        return {};
    }
    
    buffer.resize(bytes_read);
    return buffer;
}

void Connection::disconnect() noexcept {
    // Re-assigning to an empty SocketFD triggers the destructor to close the socket
    socket_ = SocketFD{}; 
}

} // namespace netsim::core