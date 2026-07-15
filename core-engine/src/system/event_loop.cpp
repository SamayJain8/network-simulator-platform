#include "system/event_loop.hpp"
#include <utility>
#include <stdexcept>

namespace netsim::system {

KqueueEventLoop::KqueueEventLoop() {
#if defined(__APPLE__)
    loop_fd_ = ::kqueue();
    if (loop_fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create kqueue");
    }
#elif defined(__linux__)
    loop_fd_ = ::epoll_create1(0);
    if (loop_fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create epoll fd");
    }
#else
    throw std::runtime_error("Unsupported Operating System for Async Event Loop");
#endif
}

KqueueEventLoop::~KqueueEventLoop() {
    if (loop_fd_ >= 0) {
        ::close(loop_fd_);
    }
}

void KqueueEventLoop::register_event(int fd, uint16_t filter, EventCallback callback) {
#if defined(__APPLE__)
    struct kevent change;
    EV_SET(&change, fd, filter, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    if (::kevent(loop_fd_, &change, 1, nullptr, 0, nullptr) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to register kevent");
    }
#elif defined(__linux__)
    struct epoll_event ev;
    ev.events = filter | EPOLLRDHUP; // EPOLLIN / Read event
    ev.data.fd = fd;
    
    int op = EPOLL_CTL_ADD;
    if (callbacks_.find(fd) != callbacks_.end()) {
        op = callback ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    }
    
    if (::epoll_ctl(loop_fd_, op, fd, &ev) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to execute epoll_ctl");
    }
#endif

    if (callback) {
        callbacks_[fd] = std::move(callback);
    } else {
        callbacks_.erase(fd);
    }
}

void KqueueEventLoop::poll_once(uint32_t timeout_ms) {
#if defined(__APPLE__)
    struct kevent events[10];
    struct timespec timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_nsec = (timeout_ms % 1000) * 1000000;

    int nevents = ::kevent(loop_fd_, nullptr, 0, events, 10, &timeout);
    if (nevents < 0) {
        if (errno == EINTR) return;
        throw std::system_error(errno, std::system_category(), "Kevent poll failed");
    }

    for (int i = 0; i < nevents; ++i) {
        int fd = static_cast<int>(events[i].ident);
        uint16_t filter = events[i].filter;

        auto it = callbacks_.find(fd);
        if (it != callbacks_.end() && it->second) {
            it->second(fd, filter);
        }
    }
#elif defined(__linux__)
    struct epoll_event events[10];
    int nevents = ::epoll_wait(loop_fd_, events, 10, timeout_ms);
    if (nevents < 0) {
        if (errno == EINTR) return;
        throw std::system_error(errno, std::system_category(), "Epoll wait failed");
    }

    for (int i = 0; i < nevents; ++i) {
        int fd = events[i].data.fd;
        uint16_t filter = events[i].events;

        auto it = callbacks_.find(fd);
        if (it != callbacks_.end() && it->second) {
            it->second(fd, filter);
        }
    }
#endif
}

} // namespace netsim::system