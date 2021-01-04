#pragma once

#include <vector>
#include <algorithm>
#include "base/threading/ThreadSafeLinearAllocator.h"

namespace cc {
namespace gfx {

namespace {
constexpr size_t DEFAULT_BLOCK_SIZE = 4096 * 16;

uint nextPowerOf2(uint v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}
}

class CC_DLL LinearAllocatorPool final {
public:
    LinearAllocatorPool(size_t defaultBlockSize = DEFAULT_BLOCK_SIZE): _defaultBlockSize(defaultBlockSize) {
        _allocators.emplace_back(CC_NEW(ThreadSafeLinearAllocator(_defaultBlockSize)));
    }

    ~LinearAllocatorPool() {
        for (ThreadSafeLinearAllocator *allocator : _allocators) {
            CC_SAFE_DELETE(allocator);
        }
        _allocators.clear();
    }

    template<typename T>
    T* allocate(const uint count, uint alignment = 64u) noexcept {
        if (!count) return nullptr;

        T* res = nullptr;
        size_t size = count * sizeof(T);
        for (ThreadSafeLinearAllocator *allocator : _allocators) {
            res = reinterpret_cast<T*>(allocator->Allocate(size, alignment));
            if (res) return res;
        }
        size_t capacity = nextPowerOf2(std::max(DEFAULT_BLOCK_SIZE, size + alignment)); // reserve enough padding space for alignment
        _allocators.emplace_back(CC_NEW(ThreadSafeLinearAllocator(capacity)));
        return reinterpret_cast<T*>(_allocators.back()->Allocate(size, alignment));
    }

    inline void reset() {
        for (ThreadSafeLinearAllocator *allocator : _allocators) {
            allocator->Recycle();
        }
    }

protected:
    vector<ThreadSafeLinearAllocator *> _allocators;
    size_t _defaultBlockSize = DEFAULT_BLOCK_SIZE;
};

} // namespace gfx
} // namespace cc
