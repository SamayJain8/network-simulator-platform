#include "core/connection.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <system_error>
#include <stdexcept>

namespace netsim::core {

Connection::Connection(SocketFD&& socket) : socket_(std::move(socket)) {
    if (socket_.is_valid()) {
        // Configure socket to be Non-Blocking (O_NONBLOCK)
        int flags = ::fcntl(socket_.get(), F_GETFL, 0);
        if (flags < 0 || ::fcntl(socket_.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
            throw std::system_error(errno, std::system_category(), "Failed to set O_NONBLOCK");
        }
    }
}

std::unique_ptr<Connection> Connection::connect_to(const std::string& ip, uint16_t port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }

    SocketFD safe_socket(sock);
    
    // Set non-blocking before connect to initiate an asynchronous TCP handshake
    int flags = ::fcntl(safe_socket.get(), F_GETFL, 0);
    if (flags < 0 || ::fcntl(safe_socket.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set O_NONBLOCK on client socket");
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (::inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::invalid_argument("Invalid IP address format: " + ip);
    }

    int rc = ::connect(safe_socket.get(), reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr));
    if (rc < 0 && errno != EINPROGRESS) {
        throw std::system_error(errno, std::system_category(), "Connection failed asynchronously");
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
    
    while (total_sent < static_cast<ssize_t>(data.size())) {
        ssize_t sent = ::send(socket_.get(), data.data() + total_sent, left_to_send, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Buffer full; yield and retry in next event loop iteration
                break;
            }
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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {}; // No data available right now; do not block
        }
        throw std::system_error(errno, std::system_category(), "Receive failed");
    } else if (bytes_read == 0) {
        disconnect(); 
        return {};
    }
    
    buffer.resize(bytes_read);
    return buffer;
}

void Connection::disconnect() noexcept {
    socket_ = SocketFD{}; 
}

} // namespace netsim::core