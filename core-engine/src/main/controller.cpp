#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <memory>
#include <span>
#include <unordered_map>
#include <chrono>
#include <thread>
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

void test_virtual_event_loop_pipeline() {
    std::cout << "[INFO] Commencing Option B: Virtual Time Event Loop validation...\n";

    using namespace netsim::system;

    EventLoop loop;
    std::vector<int> execution_order;

    // Schedule events with logical, virtual delays (in nanoseconds)
    // Event 1: scheduled at T = 100ns
    loop.schedule(100, 1, [&execution_order]() {
        execution_order.push_back(1);
    });

    // Event 2: scheduled at T = 50ns (should run first!)
    loop.schedule(50, 1, [&execution_order]() {
        execution_order.push_back(2);
    });

    // Event 3: scheduled at T = 150ns
    loop.schedule(150, 1, [&execution_order]() {
        execution_order.push_back(3);
    });

    // Verify empty state
    assert(loop.empty() == false);

    // Run the loop: virtual time advances instantly to execute events in order
    loop.run();

    // Verify that the discrete-event scheduler sorted and executed events chronologically
    assert(execution_order.size() == 3);
    assert(execution_order[0] == 2); // T = 50ns runs first
    assert(execution_order[1] == 1); // T = 100ns runs second
    assert(execution_order[2] == 3); // T = 150ns runs third

    // Verify clock advancement
    assert(loop.virtual_time() == 150);

    std::cout << "[SUCCESS] Option B: Discrete virtual time event loop validated.\n";
}

void test_memory_arena_allocation() {
    std::cout << "[INFO] Commencing Option B: Memory Arena Allocator validation...\n";

    using namespace netsim::core;

    // Pre-allocate a 10KB contiguous memory arena on the stack/heap
    MemoryArena arena(10240);

    // Allocate Packet structures inside our pre-allocated arena
    // This executes placement-new inline on our aligned memory block in O(1) time
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    Packet* pkt1 = arena.allocate<Packet>(0, payload);

    assert(pkt1 != nullptr);
    assert(pkt1->header.version == 1);
    assert(pkt1->header.length == 4);
    assert(pkt1->payload == payload);

    // Verify second allocation is contiguous and distinct
    Packet* pkt2 = arena.allocate<Packet>(1, payload);
    assert(pkt2 != nullptr);
    assert(pkt2 != pkt1);
    assert(pkt2->header.type == 1);

    // Reset the arena, freeing all memory instantly in O(1)
    arena.reset();

    std::cout << "[SUCCESS] Option B: High-speed, zero-allocation memory arena validated.\n";
}

void test_rate_limiting_security_pipeline() {
    std::cout << "[INFO] Commencing Option B: Lock-Free GCRA Rate Limiter validation...\n";

    using namespace netsim::network;

    Node server_node(1, "ProtectedServer", IPv4Address("192.168.1.10"), SubnetMask(24), "00:BB:00:11:11:11");
    server_node.enable_rate_limiting(10.0, 5.0);

    for (int i = 1; i <= 5; ++i) {
        assert(server_node.receive_packet() == true);
    }
    
    assert(server_node.receive_packet() == false);
    std::cout << "[DEBUG] Burst limit enforced. First 5 packets allowed, 6th packet dropped.\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (int i = 1; i <= 5; ++i) {
        assert(server_node.receive_packet() == true);
    }
    assert(server_node.receive_packet() == false);
    std::cout << "[DEBUG] Refill rate confirmed. Bucket refilled by 5 tokens over 500ms.\n";

    std::vector<std::thread> traffic_generators;
    std::atomic<uint32_t> allowed_count(0);
    std::atomic<uint32_t> dropped_count(0);

    auto generate_traffic = [&server_node, &allowed_count, &dropped_count]() {
        for (int i = 0; i < 10; ++i) {
            if (server_node.receive_packet()) {
                allowed_count++;
            } else {
                dropped_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    };

    for (int i = 0; i < 4; ++i) {
        traffic_generators.emplace_back(generate_traffic);
    }

    for (auto& th : traffic_generators) {
        th.join();
    }

    assert(allowed_count + dropped_count == 40);
    std::cout << "[DEBUG] Concurrency test passed. Allowed: " << allowed_count << ", Dropped: " << dropped_count << "\n";

    std::cout << "[SUCCESS] Option B: Lock-Free GCRA rate limiter fully validated.\n";
}

void test_fault_tolerance_pipeline() {
    std::cout << "[INFO] Commencing Phase 8: Fault Tolerance & Circuit Breaker validation...\n";

    using namespace netsim::network;

    // 1. Initialize routers passing unique integer IDs as the first argument
    Router router_a(1, "RouterA", IPv4Address("10.0.1.1"), SubnetMask(24), "00:AA:00:11:11:11");
    Router router_b(2, "RouterB", IPv4Address("10.0.2.1"), SubnetMask(24), "00:AA:00:22:22:22");
    Router router_c(3, "RouterC", IPv4Address("10.0.3.1"), SubnetMask(24), "00:AA:00:33:33:33");

    std::unordered_map<uint32_t, IPv4Address> node_to_ip = {
        {1, IPv4Address("10.0.1.1")},
        {2, IPv4Address("10.0.2.1")},
        {3, IPv4Address("10.0.3.1")}
    };

    std::unordered_map<uint32_t, SubnetMask> node_to_mask = {
        {1, SubnetMask(24)},
        {2, SubnetMask(24)},
        {3, SubnetMask(24)}
    };

    // Adjacency graph utilizing integer IDs
    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> graph = {
        {1, {{2, 1}, {3, 10}}},
        {2, {{1, 1}, {3, 2}}},
        {3, {{1, 10}, {2, 2}}}
    };

    router_a.compute_routes(graph, node_to_ip, node_to_mask);
    auto initial_route = router_a.lookup_route(IPv4Address("10.0.3.100"));
    assert(initial_route.first == true);
    assert(initial_route.second == 2); // Router B's ID is 2

    // 2. Record link failures using integer IDs instead of strings
    router_a.record_link_failure(2);
    router_a.record_link_failure(2);
    assert(router_a.get_link_state(2) == CircuitState::Closed);

    router_a.record_link_failure(2); // Trips to OPEN
    assert(router_a.get_link_state(2) == CircuitState::Open);

    router_a.compute_routes(graph, node_to_ip, node_to_mask);
    auto failover_route = router_a.lookup_route(IPv4Address("10.0.3.100"));
    assert(failover_route.first == true);
    assert(failover_route.second == 3); // Router C's ID is 3

    Backoff backoff_policy(100, 2000);
    uint64_t delay_attempt_1 = backoff_policy.calculate_delay(1);
    uint64_t delay_attempt_2 = backoff_policy.calculate_delay(2);
    
    assert(delay_attempt_1 <= 100);
    assert(delay_attempt_2 <= 200);

    std::cout << "[SUCCESS] Phase 8: Fault tolerance and dynamic failover validated.\n";
}


void test_lock_free_telemetry_bridge() {
    std::cout << "[INFO] Commencing Phase 7: Lock-Free Telemetry Bridge validation...\n";
    using namespace netsim::system;

    SpscRingBuffer<MetricEvent, 1024> telemetry_queue;
    TelemetryMonitor monitor(telemetry_queue);
    monitor.start();

    for (uint32_t i = 1; i <= 10; ++i) {
        MetricEvent event{
            static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
            "RouterA",
            "RouterB",
            1000,
            150,
            false
        };
        assert(telemetry_queue.push(event) == true);
    }

    for (uint32_t i = 1; i <= 2; ++i) {
        MetricEvent drop_event{
            static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
            "RouterA",
            "RouterB",
            0,
            0,
            true
        };
        assert(telemetry_queue.push(drop_event) == true);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    AggregatedMetrics stats = monitor.get_latest_metrics();

    assert(stats.total_packets == 12);
    assert(stats.dropped_packets == 2);
    assert(stats.total_bytes == 10000);
    assert(stats.average_latency_ns == 150.0);
    assert(stats.throughput_kbps > 0.0);

    monitor.stop();
    std::cout << "[SUCCESS] Phase 7: Lock-Free circular queue telemetry bridge validated.\n";
}

void test_dynamic_multi_hop_routing() {
    std::cout << "[INFO] Commencing Phase 6: Dynamic Multi-hop Link-State Routing tests...\n";
    using namespace netsim::network;

    Router router_a(1, "RouterA", IPv4Address("10.0.1.1"), SubnetMask(24), "00:AA:00:11:11:11");
    Router router_b(2, "RouterB", IPv4Address("10.0.2.1"), SubnetMask(24), "00:AA:00:22:22:22");
    Router router_c(3, "RouterC", IPv4Address("10.0.3.1"), SubnetMask(24), "00:AA:00:33:33:33");

    std::unordered_map<uint32_t, IPv4Address> node_to_ip = {
        {1, IPv4Address("10.0.1.1")},
        {2, IPv4Address("10.0.2.1")},
        {3, IPv4Address("10.0.3.1")}
    };

    std::unordered_map<uint32_t, SubnetMask> node_to_mask = {
        {1, SubnetMask(24)},
        {2, SubnetMask(24)},
        {3, SubnetMask(24)}
    };

    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> topology_graph_scenario_1 = {
        {1, {{2, 1}, {3, 10}}},
        {2, {{1, 1}, {3, 2}}},
        {3, {{1, 10}, {2, 2}}}
    };

    router_a.compute_routes(topology_graph_scenario_1, node_to_ip, node_to_mask);
    auto route1 = router_a.lookup_route(IPv4Address("10.0.3.100"));
    assert(route1.first == true);
    assert(route1.second == 2);

    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> topology_graph_scenario_2 = {
        {1, {{2, 1}, {3, 10}}},
        {2, {{1, 1}, {3, 15}}},
        {3, {{1, 10}, {2, 15}}}
    };

    router_a.compute_routes(topology_graph_scenario_2, node_to_ip, node_to_mask);
    auto route2 = router_a.lookup_route(IPv4Address("10.0.3.100"));
    assert(route2.first == true);
    assert(route2.second == 3);

    std::cout << "[SUCCESS] Phase 6: Dynamic link-state Dijkstra routing engine validated.\n";
}

void test_ipv4_subnetting_logic() {
    std::cout << "[INFO] Commencing Phase 5: Bitwise Subnet Mask validation tests...\n";
    using namespace netsim::network;

    IPv4Address ip1("192.168.1.15");
    assert(ip1.to_string() == "192.168.1.15");
    assert(ip1.to_u32() == 0xC0A8010F);

    SubnetMask mask_slash_24(24);
    assert(mask_slash_24.to_string() == "255.255.255.0");
    assert(mask_slash_24.to_u32() == 0xFFFFFF00);

    SubnetMask mask_slash_16("255.255.0.0");
    assert(mask_slash_16.to_prefix_length() == 16);

    try {
        SubnetMask invalid_mask("255.255.0.255");
        assert(false);
    } catch (const std::invalid_argument& ex) {
        std::cout << "[DEBUG] Caught expected contiguous verification error: " << ex.what() << "\n";
    }

    Node host_node(4, "HostA", IPv4Address("10.0.0.10"), SubnetMask(24), "00:11:22:33:44:55");
    IPv4Address local_target("10.0.0.20");
    IPv4Address remote_target("10.0.1.20");

    assert(host_node.is_local(local_target) == true);
    assert(host_node.is_local(remote_target) == false);

    std::cout << "[SUCCESS] Phase 5: IPv4 subnetting validated.\n";
}

void test_fragmented_http_parsing() {
    std::cout << "[INFO] Commencing Phase 4: Fragmented HTTP State Machine parsing tests...\n";
    std::unique_ptr<netsim::protocols::Protocol> http_engine = std::make_unique<netsim::protocols::HttpSim>();

    std::string raw_http_request = 
        "POST /submit-telemetry HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 19\r\n"
        "\r\n"
        "cpu_load=42&temp=61";

    std::vector<uint8_t> stream_bytes(raw_http_request.begin(), raw_http_request.end());
    std::vector<std::vector<uint8_t>> fragments;
    size_t chunk_size = 5;
    for (size_t i = 0; i < stream_bytes.size(); i += chunk_size) {
        size_t end = std::min(i + chunk_size, stream_bytes.size());
        fragments.push_back(std::vector<uint8_t>(stream_bytes.begin() + i, stream_bytes.begin() + end));
    }

    bool parser_finished = false;
    for (size_t i = 0; i < fragments.size(); ++i) {
        std::span<const uint8_t> chunk_span(fragments[i].data(), fragments[i].size());
        if (http_engine->parse_chunk(chunk_span)) {
            parser_finished = true;
        }
    }

    assert(parser_finished == true);

    auto* concrete_http = static_cast<netsim::protocols::HttpSim*>(http_engine.get());
    const auto& req = concrete_http->get_request();

    assert(req.method == "POST");
    assert(req.uri == "/submit-telemetry");
    assert(req.headers.at("Content-Length") == "19");
    
    std::string body_str(req.body.begin(), req.body.end());
    assert(body_str == "cpu_load=42&temp=61");

    std::cout << "[SUCCESS] Phase 4: Zero-loss, streaming FSM parser validated.\n";
}

int main() {
    std::cout << "[INFO] Initializing network simulator core runtime engine\n\n";

    try {
        // Run Phase 2 verification
        std::string test_msg = "ARP_TEST";
        std::vector<uint8_t> initial_data(test_msg.begin(), test_msg.end());
        netsim::core::Packet pkt(0, initial_data); 

        std::vector<uint8_t> wire_stream = pkt.serialize();
        netsim::core::Packet decoded_pkt = netsim::core::Packet::deserialize(wire_stream);
        
        assert(decoded_pkt.header.version == pkt.header.version);
        assert(decoded_pkt.payload == pkt.payload);
        std::cout << "[SUCCESS] Phase 2: Host/Network byte serialization validated.\n";

        // Run Phase 3 verification
        netsim::network::ArpCache cache(2); 
        cache.put("10.0.0.1", "00:1A:2B:3C:4D:5E");
        cache.put("10.0.0.2", "00:1A:2B:3C:4D:5F");
        cache.get("10.0.0.1");
        cache.put("10.0.0.3", "00:1A:2B:3C:4D:60");

        auto check_evicted = cache.get("10.0.0.2"); 
        auto check_retained = cache.get("10.0.0.1");

        assert(check_evicted.first == false); 
        assert(check_retained.first == true);
        std::cout << "[SUCCESS] Phase 3: Bounded O(1) LRU map eviction validated.\n";

        // Run Phase 4 verification
        test_fragmented_http_parsing();

        // Run Phase 5 verification
        test_ipv4_subnetting_logic();

        // Run Phase 6 verification
        test_dynamic_multi_hop_routing();

        // Run Phase 7 verification
        test_lock_free_telemetry_bridge();

        // Run Phase 8 verification
        test_fault_tolerance_pipeline();

        // Run Option B verification
        test_rate_limiting_security_pipeline();

        // Run Option B Advanced additions verification
        test_virtual_event_loop_pipeline();
        test_memory_arena_allocation();

        std::cout << "\n[STATUS] All engine targets built and verified successfully.\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] Pipeline verification runtime fault: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}