#ifndef MEMORYPOOL_CENTRALCACHE_H
#define MEMORYPOOL_CENTRALCACHE_H

#include "utils.h"
#include <chrono>
#include <thread>

namespace MemoryPool
{
// 帮助中心缓存管理从页缓存获取的内存块（span）
struct SpanTracker {
    std::atomic<void*> spanAddr{nullptr}; // 内存块起始地址
    std::atomic<size_t> numPages{0}; // 内存块所占页数
    std::atomic<size_t> blockCount{0}; // 内存块切成的小块数量
    std::atomic<size_t> freeCount{0}; // 空闲块的数量
};

class CentralCache {
public:
    // 只有一个中心缓存
    static CentralCache& getInstance() {
        static CentralCache instance;
        return instance;
    }
    void* fetchRange(size_t index); // 中心缓存->线程缓存
    void returnRange(void* start,size_t size,size_t index); // 线程缓存->中心缓存

private:
    CentralCache(); // 构造函数为私有 服务于单例

    // 页缓存->中心缓存
    void* fetchFromPageCache(size_t size);

    // 获取span信息
    SpanTracker* getSpanTracker(void* blockAddr);

    // 更新span的空闲计数并检查是否可以归还
    void updateSpanFreeCount(SpanTracker* tracker,size_t newFreeBlocks,size_t index);

private:
    std::array<std::atomic<void*>,FREE_LIST_SIZE> centralFreeList_; // 中心缓存的空闲链表 - 也是每种大小的内存块对应一个空闲链表 中心缓存可能会被多线程访问，因此为atomic<void*>类型

    std::array<std::atomic_flag,FREE_LIST_SIZE> locks_; // 每种大小的内存块对应空闲链表的自旋锁

    std::array<SpanTracker,1024> spanTracks_; // 使用数组存储span信息，（较map相比有更小的开销）
    std::atomic<size_t> spanCount_{0}; // 记录当前中心缓存中SpanTracker对象的数量

    static const size_t MAX_DELAY_COUNT = 48; // 最大延迟计数 - 相当于一个内存块交互的阈值
    std::array<std::atomic<size_t>,FREE_LIST_SIZE> delayCounts_; // 每种大小的内存块的延迟计数
    std::array<std::chrono::steady_clock::time_point,FREE_LIST_SIZE> lastReturnTimes_; // 上次归还时间
    static const std::chrono::milliseconds DELAY_INTERVAL; // 延迟间隔

    bool shouldPerformDelayedReturn(size_t index,size_t currentCount,std::chrono::steady_clock::time_point currentTime); // 判断指定大小类是否应执行延迟归还
    void performDelayedReturn(size_t index); // 执行指定大小类的延迟归还操作
};

} // MemoryPool

#endif //MEMORYPOOL_CENTRALCACHE_H
