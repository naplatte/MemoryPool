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
        // 内存过大，直接向系统申请
        if (index >= FREE_LIST_SIZE) {
            return nullptr;
        }
        // 自旋锁保护
        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield(); // 让出CPU，避免忙等待（相当于是带让步的自旋锁）
        }
        void* res = nullptr;

        // 尝试去中心缓存获取内存块
        try {
            res = centralFreeList_[index].load(std::memory_order_relaxed);

            if (! res) {
                // 中心缓存为空，从页缓存获取新的内存块
                size_t size = (index + 1) * ALIGNMENT;
                res = fetchFromPageCache(size);
                if (! res) {
                    // 页缓存也无法提供内存，返回空指针
                    locks_[index].clear(std::memory_order_release);
                    return nullptr;
                }

                // 将从 PageCache 获取到的一段连续内存按请求大小切分成多个块，构建空闲链表
                // 将首块作为返回结果与链表断开，并把剩余块作为 centralFreeList_ 的新头存储以供后续分配与跟踪
                char* start = static_cast<char*> (res);
                size_t numPages = (size <= SPANPAGES * PageCache::PAGE_SIZE) ?
                                    SPANPAGES :
                                  (size + 1);

            }
        }
        catch (...) {

        }

        // 释放锁
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