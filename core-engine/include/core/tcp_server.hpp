#pragma once

#include "core/connection.hpp"
#include <memory>
#include <string>

namespace netsim::core {

class TcpServer {
public:
    explicit TcpServer(uint16_t port);
    
    // Binds to the port and starts listening for traffic
    void start();
    
    // Blocks the thread until a client connects, then hands over the connection
    std::unique_ptr<Connection> accept_connection();
    
    void stop() noexcept;

    // CRITICAL FIX: Public getter to allow event loops to access the raw descriptor
    [[nodiscard]] const SocketFD& server_socket() const noexcept { return server_socket_; }

private:
    uint16_t port_;
    SocketFD server_socket_;
};

} // namespace netsim::core