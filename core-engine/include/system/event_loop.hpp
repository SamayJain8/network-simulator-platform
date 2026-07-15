#pragma once
#include <functional>
#include <unordered_map>
#include <system_error>
#include <cstdint>

// Conditional compilation based on the compiling Operating System
#if defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#define EVENT_READ EVFILT_READ // Map macOS read filter
#elif defined(__linux__)
#include <sys/epoll.h>
#include <unistd.h>
#define EVENT_READ EPOLLIN     // Map Linux read filter
#endif

namespace netsim::system {

using EventCallback = std::function<void(int fd, uint16_t filter)>;

class KqueueEventLoop { // Keep name identical to prevent breaking other files
public:
    KqueueEventLoop();
    ~KqueueEventLoop();

    KqueueEventLoop(const KqueueEventLoop&) = delete;
    KqueueEventLoop& operator=(const KqueueEventLoop&) = delete;

    void register_event(int fd, uint16_t filter, EventCallback callback);
    void poll_once(uint32_t timeout_ms);

private:
    int loop_fd_; // Stores kq_ on macOS, epoll_fd on Linux
    std::unordered_map<int, EventCallback> callbacks_;
};

} // namespace netsim::system