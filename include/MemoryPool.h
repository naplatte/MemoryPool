#ifndef MEMORYPOOL_MEMORYPOOL_H
#define MEMORYPOOL_MEMORYPOOL_H
#include "ThreadCache.h"

namespace MemoryPool
{
class MemoryPoolV2 {
public:
    static void* allocate(size_t size) {
        return ThreadCache::getInstance()->allocate(size);
    }

    static void deallocate(void* ptr,size_t size) {
        ThreadCache::getInstance()->deallocate(ptr,size);
    }
};
}


#endif //MEMORYPOOL_MEMORYPOOL_H


// 使用者
//   ↓
// MemoryPoolV2
//   ↓
// 当前线程的 ThreadCache
//   ↓ 本地没有可用内存块
// CentralCache
//   ↓
// PageCache
//   ↓
// 操作系统