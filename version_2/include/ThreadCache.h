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
        }
    };
}


#endif //MEMORYPOOL_THREADCACHE_H
