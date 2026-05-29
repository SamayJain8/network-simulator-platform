#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <memory>
#include <span>
#include "core/packet.hpp"
#include "network/arp.hpp"
#include "protocols/http_sim.hpp"
#include "network/addressing.hpp"
#include "network/node.hpp"

void test_ipv4_subnetting_logic() {
    std::cout << "[INFO] Commencing Phase 5: Bitwise Subnet Mask validation tests...\n";

    using namespace netsim::network;

    // 1. Verify safe binary parsing of dotted decimals
    IPv4Address ip1("192.168.1.15");
    assert(ip1.to_string() == "192.168.1.15");
    assert(ip1.to_u32() == 0xC0A8010F); // 192=C0, 168=A8, 1=01, 15=0F

    // 2. Verify prefix parsing logic
    SubnetMask mask_slash_24(24);
    assert(mask_slash_24.to_string() == "255.255.255.0");
    assert(mask_slash_24.to_u32() == 0xFFFFFF00);

    SubnetMask mask_slash_16("255.255.0.0");
    assert(mask_slash_16.to_prefix_length() == 16);

    // 3. Verify contiguous bit alignment limits
    try {
        SubnetMask invalid_mask("255.255.0.255"); // Non-contiguous zero boundary gap
        assert(false); // Should throw exception
    } catch (const std::invalid_argument& ex) {
        std::cout << "[DEBUG] Caught expected contiguous verification error: " << ex.what() << "\n";
    }

    // 4. Verify Local Routing evaluation checks
    Node host_node("HostA", IPv4Address("10.0.0.10"), SubnetMask(24), "00:11:22:33:44:55");

    IPv4Address local_target("10.0.0.20");
    IPv4Address remote_target("10.0.1.20");

    assert(host_node.is_local(local_target) == true);
    assert(host_node.is_local(remote_target) == false);

    std::cout << "[SUCCESS] Phase 5: IPv4 mathematical subnet matching validated.\n";
}

void test_fragmented_http_parsing() {
    std::cout << "[INFO] Commencing Phase 4: Fragmented HTTP State Machine parsing tests...\n";
    std::unique_ptr<netsim::protocols::Protocol> http_engine = std::make_unique<netsim::protocols::HttpSim>();

  std::string raw_http_request = 
        "POST /submit-telemetry HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 19\r\n"  // Changed from 18 to 19
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

        std::cout << "\n[STATUS] All engine targets built and verified successfully.\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] Pipeline verification runtime fault: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}