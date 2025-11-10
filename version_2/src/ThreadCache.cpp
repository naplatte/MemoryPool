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

        size_t index = SizeClass::getIndex(size);
        freeListSize_[index]--; // 更新对应空闲链表长度

        if (void* ptr = freeList_[index]) { // 空闲链表不为空
            freeList_[index] = * reinterpret_cast<void**>(ptr);
            return ptr;
        }
        // 空闲链表为空 - 向中心缓存申请内存

    }
}