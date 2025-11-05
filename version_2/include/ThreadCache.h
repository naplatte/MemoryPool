//
// Created by cuihs on 2025/11/4.
//

#ifndef MEMORYPOOL_THREADCACHE_H
#define MEMORYPOOL_THREADCACHE_H
#include "utils.h"


namespace MemoryPool {
    // 线程本地缓存
    class ThreadCache {
    public:
        static ThreadCache* getInstance() {
            static thread_local ThreadCache instance;
            return &instance;
        }

        // 为线程分配内存
        void* allocate(size_t size);

        // 处理线程释放内存
        void deallocate(void* ptr,size_t size);

    private:
        // 单例 构造设为private
        ThreadCache() {
            // 初始化空闲链表和大小统计(初始空闲链表为空)
            freeList_.fill(nullptr);
            freeListSize_.fill(0);
        }

    private:
        // 每个线程都有一个空闲链表数组
        std::array<void*,FREE_LIST_SIZE> freeList_;
        std::array<size_t,FREE_LIST_SIZE> freeListSize_;

    };
}


#endif //MEMORYPOOL_THREADCACHE_H
