//
// Created by cuihs on 2025/11/18.
//

#include "../include/CentralCache.h"

namespace MemoryPool {
    CentralCache::CentralCache() {
        // 所有大小的内存块对应的空闲链表，初始为空 memory_order_relaxed - 不保证顺序，仅保证原子性
        for (auto& ptr : centralFreeList_) {
            ptr.store(nullptr,std::memory_order_relaxed);
        }

    }

    void *CentralCache::fetchRange(size_t index) {
        return nullptr;
    }

    void CentralCache::returnRange(void *start, size_t size, size_t index) {

    }

    void *CentralCache::fetchFromPageCache(size_t size) {
        return nullptr;
    }

    SpanTracker *CentralCache::getSpanTracker(void *blockAddr) {
        return nullptr;
    }

    void CentralCache::updateSpanFreeCount(SpanTracker *tracker, size_t newFreeBlocks, size_t index) {

    }

    bool CentralCache::shouldPerformDelayedReturn(size_t index, size_t currentCount,
                                                  std::chrono::steady_clock::time_point currentTime) {
        return false;
    }

    void CentralCache::performDelayedReturn(size_t index) {

    }


} // MemoryPool