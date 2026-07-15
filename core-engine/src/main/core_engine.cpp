#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <cstring>
#include "core/packet.hpp"
#include "network/arp.hpp"
#include "protocols/http_sim.hpp"
#include "network/addressing.hpp"
#include "network/node.hpp"
#include "network/router.hpp"
#include "system/event_queue.hpp"
#include "system/monitor.hpp"
#include "network/backoff.hpp"
#include "system/event_loop.hpp"
#include "core/memory_arena.hpp"
#include "core/tcp_server.hpp"

using namespace netsim::core;
using namespace netsim::network;
using namespace netsim::system;
using namespace netsim::protocols;

// Represents a client's context, pairing the connection with its dedicated FSM parser
struct ClientContext {
    std::unique_ptr<Connection> connection;
    HttpSim parser;
};

// Global components for our persistent running server
SpscRingBuffer<MetricEvent, 1024> g_telemetry_queue;
std::unique_ptr<TelemetryMonitor> g_monitor;
std::unique_ptr<MemoryArena> g_arena;

// Map to retain active connection states and FSM parsers
std::unordered_map<int, ClientContext> g_active_connections;

int main() {
    std::cout << "============================================================\n";
    std::cout << "[INFO] Launching persistent Asynchronous Routing Engine Daemon\n";
    std::cout << "============================================================\n";

    try {
        // Initialize our active server architecture
        TcpServer main_server(9090);
        main_server.start();
        std::cout << "[SERVER] Non-blocking listening socket bound to port 9090.\n";

        // Initialize our SPSC Telemetry Queue & Background Monitor Thread
        g_monitor = std::make_unique<TelemetryMonitor>(g_telemetry_queue);
        g_monitor->start();
        std::cout << "[SERVER] Telemetry background monitor thread started.\n";

        // Initialize our high-speed Packet Memory Arena (20KB)
        g_arena = std::make_unique<MemoryArena>(20480); 
        std::cout << "[SERVER] Zero-allocation Memory Arena initialized (20KB).\n";

        // Initialize our secure, lock-free GCRA Rate-Limited Host Node
        Node secure_node(1, "SecureHost", IPv4Address("127.0.0.1"), SubnetMask(24), "00:BB:00:11:11:11");
        secure_node.enable_rate_limiting(5.0, 3.0); // 5 packets/sec, burst capacity of 3
        std::cout << "[SERVER] Rate-limiting protection active (GCRA: 5 pkt/s, burst 3).\n";

        // Initialize our kqueue event multiplexer loop
        KqueueEventLoop main_loop;

        // Register the listening server socket on the kqueue event loop
       // 2. Register the listening server socket on the event loop
        main_loop.register_event(main_server.server_socket().get(), EVENT_READ, [&](int fd, uint16_t filter) {
            while (auto client_conn = main_server.accept_connection()) {
                int client_fd = client_conn->socket().get();
                std::cout << "[SERVER] Accepted new asynchronous client connection on FD: " << client_fd << "\n";

                g_active_connections[client_fd] = ClientContext{std::move(client_conn), HttpSim{}};

                // Register this client socket on our event loop to poll for reads
                main_loop.register_event(client_fd, EVENT_READ, [&](int c_fd, uint16_t c_filter) {
                    auto conn_it = g_active_connections.find(c_fd);
                    if (conn_it == g_active_connections.end()) return;

                    auto& client_ctx = conn_it->second;
                    std::vector<uint8_t> raw_bytes = client_ctx.connection->receive_data(2048);

                    if (raw_bytes.empty()) {
                        // Client cleanly closed connection or socket is disconnected
                        std::cout << "[SERVER] Client on FD " << c_fd << " disconnected. Reclaiming resources.\n";
                        g_active_connections.erase(conn_it);
                        return;
                    }

                    // Feed raw bytes to the connection's dedicated FSM parser chunk-by-chunk
                    // This resolves the "Sticky Packet" (TCP fragmentation) issue
                    std::span<const uint8_t> byte_span(raw_bytes.data(), raw_bytes.size());
                    
                    if (client_ctx.parser.parse_chunk(byte_span)) {
                        // We have parsed a complete, valid transaction boundary!
                        const HttpRequest& req = client_ctx.parser.get_request();
                        
                        try {
                            // Fast, zero-allocation parse of the packet within our pre-allocated memory arena
                            Packet* pkt = g_arena->allocate<Packet>();
                            *pkt = Packet::deserialize(req.body);

                            // Check the lock-free GCRA rate limiter
                            if (secure_node.receive_packet()) {
                                std::cout << "[SERVER] Allowed packet from FD " << c_fd 
                                          << " (Payload size: " << pkt->header.length << " bytes)\n";

                                // Stream successful packet metrics to our SPSC queue asynchronously
                                MetricEvent event{
                                    static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
                                    "ClientFD_" + std::to_string(c_fd),
                                    "SecureHost",
                                    static_cast<uint32_t>(req.body.size()),
                                    120, // Simulated network latency (ns)
                                    false // Not dropped
                                };
                                g_telemetry_queue.push(event);
                            } else {
                                std::cout << "[SERVER] RATE LIMIT BREACH on FD " << c_fd << "! Dropping packet.\n";

                                // Stream dropped packet metrics to our SPSC queue asynchronously
                                MetricEvent event{
                                    static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
                                    "ClientFD_" + std::to_string(c_fd),
                                    "SecureHost",
                                    0,
                                    0,
                                    true // Dropped flag set
                                };
                                g_telemetry_queue.push(event);
                            }

                        } catch (const std::exception& ex) {
                            std::cerr << "[SERVER] Malformed packet payload on FD " << c_fd << ": " << ex.what() << "\n";
                        }

                        // Reset the memory arena instantly in O(1) clock cycles for the next packet event
                        g_arena->reset();
                        // Reset the parser state machine for the next transaction on this connection
                        client_ctx.parser.reset();
                    }
                });
            }
        });

        std::cout << "[SERVER] Event loop initialized. Listening for active traffic events...\n\n";

        // Run the persistent, non-blocking event loop indefinitely
        while (true) {
            main_loop.poll_once(500); // Poll once every 500ms
        }

    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] Server daemon crashed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}