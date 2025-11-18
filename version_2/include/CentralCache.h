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


};

} // MemoryPool

#endif //MEMORYPOOL_CENTRALCACHE_H
