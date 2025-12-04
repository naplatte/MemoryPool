//
// Created by cuihs on 2025/11/18.
//
#include "../include/CentralCache.h"
#include "../include/PageCache.h"

namespace MemoryPool {
    static const size_t SPANPAGES = 8; // 每次从页缓存获取的span大小（是一个8页的区间）

    CentralCache::CentralCache() {
        // 所有大小的内存块对应的空闲链表，初始为空 memory_order_relaxed - 不保证顺序，仅保证原子性
        for (auto& ptr : centralFreeList_) {
            ptr.store(nullptr,std::memory_order_relaxed);
        }

    }

    void *CentralCache::fetchRange(size_t index) {
        if (index >= FREE_LIST_SIZE)
            return nullptr;
        // 自旋锁 - test and set
        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield(); // 线程让出CPU，避免忙等占用资源
        }

        // 退出while说明获得了所需大小内存块对应空闲链表的使用权
        void* res = nullptr;
        try {
            res = centralFreeList_[index].load(std::memory_order_relaxed);
        }
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