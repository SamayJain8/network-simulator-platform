#pragma once
#include <functional>
#include <queue>
#include <vector>
#include <cstdint>

namespace netsim::system {

using EventCallback = std::function<void()>;

struct Event {
    uint64_t virtual_time_ns;
    uint32_t priority;
    EventCallback callback;

    // Min-heap ordering: sorts events by virtual execution time first, priority second
    bool operator>(const Event& other) const noexcept {
        if (virtual_time_ns == other.virtual_time_ns) {
            return priority > other.priority;
        }
        return virtual_time_ns > other.virtual_time_ns;
    }
};

class EventLoop {
public:
    EventLoop() noexcept;

    // Schedules a new event in the future virtual timeline
    void schedule(uint64_t delay_ns, uint32_t priority, EventCallback callback);

    // Drives the virtual time clock forward, executing all scheduled events sequentially
    void run() noexcept;

    [[nodiscard]] uint64_t virtual_time() const noexcept { return virtual_time_ns_; }
    [[nodiscard]] bool empty() const noexcept { return event_queue_.empty(); }
    void reset() noexcept;

private:
    uint64_t virtual_time_ns_;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> event_queue_;
};

} // namespace netsim::system