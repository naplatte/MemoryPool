//
// Created by cuihs on 2025/11/4.
//

#ifndef MEMORYPOOL_THREADCACHE_H
#define MEMORYPOOL_THREADCACHE_H
#include "utils.h"

// 理清这个类：首先，所有线程都会有一个自己独立的ThreadCache，线程从这里申请内存，这样每个线程都操作一个自己的缓存，就不需要锁
// ThreadCache对线程的操作，放在public中（线程可以通过调用线程缓存对象的函数来申请和释放内存）
// ThreadCache对中心缓存的操作，放在private中，只有类内部的代码可以访问（这里写到那再回来细品）

namespace MemoryPool
{
// 线程本地缓存
class ThreadCache {
public:
    static ThreadCache* getInstance() {
        static thread_local ThreadCache instance; // 每个线程都会有一个ThreadCache对象（副本）
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

    // 从中心缓存获取内存
    void* fetchFromCentralCache(size_t index);

    // 归还内存到中心缓存
    void returnToCentralCache(void* start,size_t size);

    bool shouldReturnToCentralCache(size_t index);

private:
    std::array<void*,FREE_LIST_SIZE> freeList_; // 每个线程都维护一个空闲链表
    std::array<size_t,FREE_LIST_SIZE> freeListSize_; // 每种大小的空闲内存块数量
};
}


#endif //MEMORYPOOL_THREADCACHE_H
