#include "system/event_loop.hpp"
#include <utility>

namespace netsim::system {

EventLoop::EventLoop() noexcept : virtual_time_ns_(0) {}

void EventLoop::schedule(uint64_t delay_ns, uint32_t priority, EventCallback callback) {
    uint64_t execution_time = virtual_time_ns_ + delay_ns;
    event_queue_.push(Event{execution_time, priority, std::move(callback)});
}

void EventLoop::run() noexcept {
    while (!event_queue_.empty()) {
        Event current_event = event_queue_.top();
        event_queue_.pop();

        // Advance the virtual clock instantly to match the discrete event execution time
        virtual_time_ns_ = current_event.virtual_time_ns;
        
        // Execute callback
        if (current_event.callback) {
            current_event.callback();
        }
    }
}

void EventLoop::reset() noexcept {
    virtual_time_ns_ = 0;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> empty_queue;
    std::swap(event_queue_, empty_queue);
}

} // namespace netsim::system