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
        if (size > MAX_BYTES) {
            return malloc(size); // 大内存直接系统调用
        }

        size_t index = SizeClass::getIndex(size);

        // 对应大小内存块还有空闲内存
        if (void* ptr = freeList_[index]) {
            freeList_[index] = *(void**)ptr; // void**是指向void*的指针，*(void**)就是对该指针解引用（取出next - 即指向下一内存块的地址）
            freeListSize_[index]--;
            return ptr;
        }

        // (else)空闲链表为空 - 向中心缓存申请内存
        return fetchFromCentralCache(index);
    }

    void *ThreadCache::fetchFromCentralCache(size_t index) {


        return nullptr;
    }
}

