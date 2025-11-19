#ifndef MEMORYPOOL_CENTRALCACHE_H
#define MEMORYPOOL_CENTRALCACHE_H

#include "utils.h"

namespace MemoryPool
{

class CentralCache {
public:
    // 只有一个中心缓存
    static CentralCache& getInstance() {
        static CentralCache instance;
        return instance;
    }
    // 从中心缓存获得内存块
    void* fetchRange(size_t index);
    void returnRange(void* start,size_t size,size_t index);

private:
    CentralCache(); // 构造函数为私有 服务于单例

    // 从页缓存获取内存
    void* fetchFromPageCache(size_t size);

private:
    std::array<std::atomic<void*>,FREE_LIST_SIZE> centralFreeList_; // 中心缓存的空闲链表 - 也是每种大小的内存块对应一个空闲链表 中心缓存可能会被多线程访问，因此为atomic<void*>类型



};

} // MemoryPool

#endif //MEMORYPOOL_CENTRALCACHE_H
