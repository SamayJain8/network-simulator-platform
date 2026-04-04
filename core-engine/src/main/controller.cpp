#include <iostream>
#include "core/tcp_server.hpp"

int main() {
    std::cout << "🌐 Distributed Network Core initialized successfully." << std::endl;
    
    try {
        // Boot up a server on port 8080
        netsim::core::TcpServer server(8080);
        server.start();
        std::cout << "🚀 Server listening on port 8080... (Waiting for client)" << std::endl;
        
        // The program will pause here until someone connects
        auto client_conn = server.accept_connection();
        
        std::cout << "✅ Client connected successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
    }

    return 0;
}