#include "system/event_loop.hpp"

namespace netsim::system {

KqueueEventLoop::KqueueEventLoop() {
    kq_ = ::kqueue();
    if (kq_ < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create kqueue");
    }
}

KqueueEventLoop::~KqueueEventLoop() {
    if (kq_ >= 0) {
        ::close(kq_);
    }
}

void KqueueEventLoop::register_event(int fd, uint16_t filter, EventCallback callback) {
    struct kevent change;
    // Set up a kqueue event structure to monitor the socket file descriptor
    EV_SET(&change, fd, filter, EV_ADD | EV_ENABLE, 0, 0, nullptr);

    if (::kevent(kq_, &change, 1, nullptr, 0, nullptr) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to register kevent");
    }

    callbacks_[fd] = std::move(callback);
}

void KqueueEventLoop::poll_once(uint32_t timeout_ms) {
    struct kevent events[10];
    struct timespec timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_nsec = (timeout_ms % 1000) * 1000000;

    // Retrieve active events from the kernel
    int nevents = ::kevent(kq_, nullptr, 0, events, 10, &timeout);
    if (nevents < 0) {
        if (errno == EINTR) return; // Interrupted by system call; safe to return
        throw std::system_error(errno, std::system_category(), "Kevent poll failed");
    }

    for (int i = 0; i < nevents; ++i) {
        int fd = static_cast<int>(events[i].ident);
        uint16_t filter = events[i].filter;

        auto it = callbacks_.find(fd);
        if (it != callbacks_.end() && it->second) {
            // Trigger the non-blocking callback
            it->second(fd, filter);
        }
    }
}

} // namespace netsim::system