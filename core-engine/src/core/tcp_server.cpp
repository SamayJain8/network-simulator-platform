#include "core/tcp_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <system_error>

namespace netsim::core {

TcpServer::TcpServer(uint16_t port) : port_(port) {}

void TcpServer::start() {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::system_error(errno, std::system_category(), "Failed to create server socket");
    server_socket_ = SocketFD(sock);

    int opt = 1;
    if (::setsockopt(server_socket_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set SO_REUSEADDR");
    }

    // Set server listening socket to Non-Blocking
    int flags = ::fcntl(server_socket_.get(), F_GETFL, 0);
    if (flags < 0 || ::fcntl(server_socket_.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set O_NONBLOCK on server socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (::bind(server_socket_.get(), reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::system_error(errno, std::system_category(), "Bind failed on port " + std::to_string(port_));
    }

    if (::listen(server_socket_.get(), 10) < 0) {
        throw std::system_error(errno, std::system_category(), "Listen failed");
    }
}

std::unique_ptr<Connection> TcpServer::accept_connection() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    
    // In Non-Blocking mode, accept returns immediately with -1 if no pending connections exist
    int client_sock = ::accept(server_socket_.get(), reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    
    if (client_sock < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return nullptr; // No pending connection
        }
        throw std::system_error(errno, std::system_category(), "Accept failed");
    }

    return std::make_unique<Connection>(SocketFD(client_sock));
}

void TcpServer::stop() noexcept {
    server_socket_ = SocketFD{};
}

} // namespace netsim::core