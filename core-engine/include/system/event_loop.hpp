#pragma once
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>
#include <system_error>
#include <cstdint>

namespace netsim::system {

using EventCallback = std::function<void(int fd, uint16_t filter)>;

class KqueueEventLoop {
public:
    KqueueEventLoop();
    ~KqueueEventLoop();

    // Disable copy semantics
    KqueueEventLoop(const KqueueEventLoop&) = delete;
    KqueueEventLoop& operator=(const KqueueEventLoop&) = delete;

    // Registers a socket file descriptor for read or write events in kqueue
    void register_event(int fd, uint16_t filter, EventCallback callback);

    // Polls for events and executes their callbacks asynchronously
    void poll_once(uint32_t timeout_ms);

private:
    int kq_;
    std::unordered_map<int, EventCallback> callbacks_;
};

} // namespace netsim::system