//
// Created by cuihs on 2025/11/4.
//

#include <cstdlib>
#include "../include/ThreadCache.h"
namespace MemoryPool
{

    void *ThreadCache::allocate(size_t size) {
        if (size == 0) {
            size = ALIGNMENT; // 至少分配一个对齐大小(8B)
        }
        if (size > MAX_BTTES) {
            return malloc(size); // 大内存直接系统调用
        }


    }
}