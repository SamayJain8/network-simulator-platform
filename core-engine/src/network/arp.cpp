#include "network/arp.hpp"
#include <chrono>

namespace netsim::network {

ArpCache::ArpCache(size_t max_capacity) : capacity_limit(max_capacity) {
    if (capacity_limit == 0) {
        throw std::invalid_argument("Cache capacity bounds must exceed zero allocations.");
    }
}

std::pair<bool, std::string> ArpCache::get(const std::string& ip) {
    auto map_it = cache_map.find(ip);
    
    // Cache Miss Handshake
    if (map_it == cache_map.end()) {
        return {false, ""};
    }

    // Cache Hit Mutation: Splice tracked reference directly to list head (Most Recently Used)
    // std::list::splice shifts nodes in O(1) via pointer swapping, preventing data copies
    usage_list.splice(usage_list.begin(), usage_list, map_it->second.list_position);
    
    return {true, map_it->second.mac_address};
}

void ArpCache::put(const std::string& ip, const std::string& mac) {
    auto map_it = cache_map.find(ip);

    if (map_it != cache_map.end()) {
        // Entry Update Scenario: Mutate existing physical data address entries
        map_it->second.mac_address = mac;
        // Elevate usage hierarchy rank status to head position
        usage_list.splice(usage_list.begin(), usage_list, map_it->second.list_position);
        return;
    }

    // Capacity Threshold Enforcement: Evict long-standing items if structure is saturated
    if (cache_map.size() >= capacity_limit) {
        // Isolate the least recently used element sitting at the back of the list
        std::string eviction_target_key = usage_list.back();
        
        // Sever matching records from both data structures completely
        cache_map.erase(eviction_target_key);
        usage_list.pop_back();
    }

    // New Entry Insertion: Push tracking item to list head allocation context
    usage_list.push_front(ip);
    
    // Lock key allocations into our hash map structure along with its positional iterator shortcut
    cache_map[ip] = CacheNode{mac, usage_list.begin()};
}

bool ArpCache::invalidate(const std::string& ip) {
    auto map_it = cache_map.find(ip);
    if (map_it == cache_map.end()) {
        return false;
    }

    // Erase structural references sequentially across both layout tracking metrics
    usage_list.erase(map_it->second.list_position);
    cache_map.erase(map_it);
    return true;
}

} // namespace netsim::network
