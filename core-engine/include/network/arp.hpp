#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <vector>
#include <stdexcept>
#include <utility>
#include <cstdint> // <--- CRITICAL PORTABILITY INCLUDE: Required for uint64_t on Linux

namespace netsim::network {

// Structure holding physical identity details and lifecycle tracking metrics
struct ArpEntry {
    std::string ip_address;
    std::string mac_address;
    uint64_t timestamp; // Tracker tracking entry generation limits
};

class ArpCache {
public:
    // Enforce explicit boundary allocations to defend engine against memory corruption
    explicit ArpCache(size_t max_capacity = 100);

    // Map an IP address to a known physical MAC address structure mapping
    void put(const std::string& ip, const std::string& mac);

    // Retrieve physical address matching log mapping boundaries
    // Returns pair: <bool found, std::string mac_address>
    std::pair<bool, std::string> get(const std::string& ip);

    // Evict a specific targeted entry forcefully (simulation mutations)
    bool invalidate(const std::string& ip);

    // Diagnostic utilities to inspect internal structure allocations
    [[nodiscard]] size_t size() const { return cache_map.size(); }
    [[nodiscard]] size_t capacity() const { return capacity_limit; }

private:
    size_t capacity_limit;

    // The Doubly Linked List tracking chronology usage flow (Most Recent at Head, Stagnant at Tail)
    std::list<std::string> usage_list;

    // Iterator cross-reference definitions mapping keys to specific list positions for O(1) mutations
    using ListIterator = std::list<std::string>::iterator;

    // Internal tracker wrapping our key address data mappings along with positional shortcuts
    struct CacheNode {
        std::string mac_address;
        ListIterator list_position;
    };

    // Fast lookup translation map table wrapper
    std::unordered_map<std::string, CacheNode> cache_map;
};

} // namespace netsim::network