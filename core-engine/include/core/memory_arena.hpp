#pragma once
#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <utility>
#include <memory>  // <--- CRITICAL FIX: Required for std::align!

namespace netsim::core {

class MemoryArena {
public:
    /**
     * @brief Construct a high-speed Memory Arena.
     * 
     * @param bytes_capacity Size of the pre-allocated contiguous memory block.
     */
    explicit MemoryArena(size_t bytes_capacity) {
        // Pre-allocate contiguous memory on startup
        // Aligning to 64 bytes for cache friendliness and hardware efficiency
        arena_start_ = static_cast<uint8_t*>(::operator new(bytes_capacity, std::align_val_t{64}));
        arena_end_ = arena_start_ + bytes_capacity;
        current_cursor_ = arena_start_;
    }

    ~MemoryArena() {
        ::operator delete(arena_start_, std::align_val_t{64});
    }

    // Disable copy/move semantics to maintain memory safety
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    /**
     * @brief Fast O(1) Bump Allocation with Placement New.
     * Just increments the cursor pointer by the aligned object size.
     */
    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        constexpr size_t size = sizeof(T);
        constexpr size_t alignment = alignof(T);

        // Align the current cursor pointer
        void* ptr = current_cursor_;
        size_t space = arena_end_ - current_cursor_;

        if (!std::align(alignment, size, ptr, space)) {
            throw std::bad_alloc(); // Arena space exhausted
        }

        current_cursor_ = static_cast<uint8_t*>(ptr) + size;

        // Construct the object directly inside the pre-allocated memory block
        return ::new (ptr) T(std::forward<Args>(args)...);
    }

    // Resets the cursor back to the beginning of the pre-allocated block in O(1)
    void reset() noexcept {
        current_cursor_ = arena_start_;
    }

private:
    uint8_t* arena_start_;
    uint8_t* arena_end_;
    uint8_t* current_cursor_;
};

} // namespace netsim::core