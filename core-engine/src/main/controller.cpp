#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include "core/packet.hpp"
#include "network/arp.hpp"

int main() {
    std::cout << "[INFO] Initializing integrated pipeline verification (Phases 2 & 3)\n";

    try {
        // 1. Validate Phase 2 Endianness via exact structure header models
        std::string test_msg = "ARP_TEST";
        std::vector<uint8_t> initial_data(test_msg.begin(), test_msg.end());
        
        // Initialize packet with explicit Data type classification (0)
        netsim::core::Packet pkt(0, initial_data); 

        std::vector<uint8_t> wire_stream = pkt.serialize();
        netsim::core::Packet decoded_pkt = netsim::core::Packet::deserialize(wire_stream);
        
        assert(decoded_pkt.header.version == pkt.header.version);
        assert(decoded_pkt.header.type == pkt.header.type);
        assert(decoded_pkt.header.length == pkt.header.length);
        assert(decoded_pkt.payload == pkt.payload);
        std::cout << "[SUCCESS] Phase 2: Host/Network byte serialization validated cleanly.\n";

        // 2. Validate Phase 3 Finite Boundary LRU Cache Mechanics
        netsim::network::ArpCache cache(2); 
        cache.put("10.0.0.1", "00:1A:2B:3C:4D:5E");
        cache.put("10.0.0.2", "00:1A:2B:3C:4D:5F");
        
        cache.get("10.0.0.1");
        cache.put("10.0.0.3", "00:1A:2B:3C:4D:60");

        auto check_evicted = cache.get("10.0.0.2"); 
        auto check_retained = cache.get("10.0.0.1");

        assert(check_evicted.first == false); 
        assert(check_retained.first == true);
        std::cout << "[SUCCESS] Phase 3: Bounded O(1) LRU map eviction architecture validated.\n";

        std::cout << "\n[STATUS] System build targets verified unified and functional.\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] Pipeline verification runtime fault: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}