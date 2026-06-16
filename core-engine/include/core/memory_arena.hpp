#pragma once
#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <utility>
#include <memory>
#include <type_traits>

namespace netsim::core {

class MemoryArena {
public:
    explicit MemoryArena(size_t bytes_capacity) {
        arena_start_ = static_cast<uint8_t*>(::operator new(bytes_capacity, std::align_val_t{64}));
        arena_end_ = arena_start_ + bytes_capacity;
        current_cursor_ = arena_start_;
    }

    ~MemoryArena() {
        ::operator delete(arena_start_, std::align_val_t{64});
    }

    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    /**
     * @brief Fast O(1) Bump Allocation.
     * Enforces that only trivially destructible types are allowed to prevent heap leaks on reset.
     */
    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        static_assert(std::is_trivially_destructible_v<T>, 
            "Arena allocations are restricted to Trivially Destructible types to prevent memory leaks on reset");

        constexpr size_t size = sizeof(T);
        constexpr size_t alignment = alignof(T);

        void* ptr = current_cursor_;
        size_t space = arena_end_ - current_cursor_;

        if (!std::align(alignment, size, ptr, space)) {
            throw std::bad_alloc();
        }

        current_cursor_ = static_cast<uint8_t*>(ptr) + size;
        return ::new (ptr) T(std::forward<Args>(args)...);
    }

    void reset() noexcept {
        current_cursor_ = arena_start_;
    }

private:
    uint8_t* arena_start_;
    uint8_t* arena_end_;
    uint8_t* current_cursor_;
};

} // namespace netsim::core