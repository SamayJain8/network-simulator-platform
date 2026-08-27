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
#include "system/telemetry_http_client.hpp"
#include "network/backoff.hpp"
#include "system/event_loop.hpp"
#include "core/memory_arena.hpp"
#include "core/tcp_server.hpp"
#include <cctype>
#include <cstdlib>

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
std::unique_ptr<TelemetryHttpClient> g_telemetry_client;

// Map to retain active connection states and FSM parsers
std::unordered_map<int, ClientContext> g_active_connections;

uint64_t current_timestamp_us() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

bool env_flag_enabled(const char* name, bool default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return default_value;
    }

    std::string normalized(value);
    for (char& ch : normalized) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

void publish_metric_event(const MetricEvent& event, double throughput_kbps) {
    if (!g_telemetry_client) {
        return;
    }

    const bool published = g_telemetry_client->post_metric_event(event, throughput_kbps);
    if (!published) {
        std::cerr << "[TELEMETRY] Failed to publish packet metric to "
                  << g_telemetry_client->endpoint_url() << "\n";
    }
}

void run_demo_traffic_publisher() {
    std::cout << "[DEMO] C++ telemetry scenario publisher started.\n";

    uint32_t tick = 0;
    while (true) {
        const bool congestion_window = (tick % 18) >= 9;
        const double router_b_c_latency = congestion_window ? 430.0 + ((tick % 3) * 35.0) : 145.0;
        const uint32_t router_b_c_drops = congestion_window ? 3U : 0U;
        const double router_b_c_throughput = congestion_window ? 480.0 : 1250.0;

        std::vector<TelemetrySample> samples{
            {
                current_timestamp_us(),
                "RouterA",
                "RouterB",
                1000,
                125.0 + static_cast<double>(tick % 4) * 4.0,
                0,
                1320.0
            },
            {
                current_timestamp_us(),
                "RouterB",
                "RouterC",
                congestion_window ? 760U : 1000U,
                router_b_c_latency,
                router_b_c_drops,
                router_b_c_throughput
            },
            {
                current_timestamp_us(),
                "RouterA",
                "RouterC",
                1000,
                congestion_window ? 165.0 : 118.0,
                0,
                congestion_window ? 1420.0 : 1180.0
            }
        };

        for (const auto& sample : samples) {
            const bool published = g_telemetry_client && g_telemetry_client->post_sample(sample);
            if (published) {
                std::cout << "[TELEMETRY] Published "
                          << sample.source_node << "->" << sample.dest_node
                          << " latency=" << sample.latency_ns
                          << "ns drops=" << sample.dropped_packets << "\n";
            } else if (tick % 5 == 0) {
                std::cerr << "[TELEMETRY] Waiting for AI service at "
                          << (g_telemetry_client ? g_telemetry_client->endpoint_url() : "unconfigured endpoint")
                          << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        ++tick;
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
    }
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

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

        const char* endpoint_env = std::getenv("NETSIM_TELEMETRY_ENDPOINT");
        const std::string telemetry_endpoint = endpoint_env != nullptr
            ? endpoint_env
            : "http://127.0.0.1:8000/telemetry/stream";
        g_telemetry_client = std::make_unique<TelemetryHttpClient>(telemetry_endpoint);
        std::cout << "[SERVER] Telemetry publisher configured for "
                  << g_telemetry_client->endpoint_url() << "\n";

        if (env_flag_enabled("NETSIM_DEMO_TRAFFIC", true)) {
            std::thread(run_demo_traffic_publisher).detach();
        } else {
            std::cout << "[DEMO] C++ telemetry scenario publisher disabled.\n";
        }

        // Initialize our high-speed Packet Memory Arena (20KB)
        g_arena = std::make_unique<MemoryArena>(20480); 
        std::cout << "[SERVER] Zero-allocation Memory Arena initialized (20KB).\n";

        // Initialize our secure, lock-free GCRA Rate-Limited Host Node
        Node secure_node(1, "SecureHost", IPv4Address("127.0.0.1"), SubnetMask(24), "00:BB:00:11:11:11");
        secure_node.enable_rate_limiting(5.0, 3.0); // 5 packets/sec, burst capacity of 3
        std::cout << "[SERVER] Rate-limiting protection active (GCRA: 5 pkt/s, burst 3).\n";

        // Initialize the native event multiplexer loop (kqueue on macOS, epoll on Linux)
        KqueueEventLoop main_loop;

        // Register the listening server socket on the event loop
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
                                publish_metric_event(event, static_cast<double>(req.body.size() * 8));
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
                                publish_metric_event(event, 0.0);
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
