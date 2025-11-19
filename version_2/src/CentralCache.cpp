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


} // MemoryPool