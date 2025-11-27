//
// Created by cuihs on 2025/11/4.
//

#include <cstdlib>
#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"

namespace MemoryPool
{
    void *ThreadCache::allocate(size_t size) {
        if (size == 0) {
            size = ALIGNMENT; // 至少分配一个对齐大小(8B)
        }
        if (size > MAX_BYTES) {
            return malloc(size); // 大内存直接系统调用
        }

        size_t index = SizeClass::getIndex(size); // index就是freeList_数组的下标，表示“多大“的内存块

        // 对应大小内存块还有空闲内存
        if (void* ptr = freeList_[index]) {
            freeList_[index] = *(void**)ptr; // void**是指向void*的指针，*(void**)就是对该指针解引用（取出next - 即指向下一内存块的地址）
            freeListSize_[index]--;
            return ptr;
        }

        // (else)空闲链表为空 - 向中心缓存申请内存
        return fetchFromCentralCache(index);
    }

    void ThreadCache::deallocate(void *ptr, size_t size) {
        if (size > MAX_BYTES) {
            free(ptr);
            return;
        }

        size_t index = SizeClass::getIndex(size);

        // 对于指针来说，需要各自修改指向
        *reinterpret_cast<void**>(ptr) = freeList_[index]; // 将对应大小的空闲链表首地址 赋给 即将被释放的内存块地址
        freeList_[index] = ptr; // ptr赋值给空闲链表首地址
        freeListSize_[index]++;

        // 判断是否需要将内存回收给中心缓存
        if (shouldReturnToCentralCache(index)) {
            returnToCentralCache(freeList_[index],size);
        }
    }

    void *ThreadCache::fetchFromCentralCache(size_t index) {
        // 调用中心缓存取内存块的接口，获得指向内存块的指针
        void* start = CentralCache::getInstance().fetchRange(index);
        // 中心缓存未能提供内存块,返回空指针
        if (! start)
            return nullptr;

        void* res = start;
        freeList_[index] = *reinterpret_cast<void**>(start); // 将指针所指的第一块内存分给申请的线程

        // 统计本次申请的内存数量，以及全部放入空闲链表中
        size_t num = 0;
        void* cur = start; // 从start开始没问题，返回的res也要放在空闲链表中
        while (cur != nullptr) {
            num++;
            cur = *reinterpret_cast<void**> (cur); // cur就是指向下一个内存块的地址，解引用实际上就是cur遍历下一内存块
        }
        // 更新该大小内存块对应的该线程的空闲链表
        freeListSize_[index] += num;

        return res;
    }

    bool ThreadCache::shouldReturnToCentralCache(size_t index) {
        // 设定阈值，当对应大小的内存块空闲链表的空闲块数量大于阈值时，将内存回收
        size_t threshold = 256;
        return (freeListSize_[index] > threshold);
    }

    void ThreadCache::returnToCentralCache(void *start, size_t size) {
        size_t index = SizeClass::getIndex(size);
        size_t realSize = SizeClass::roundUp(size); // 算上对齐填充部分的实际内存块大小
        size_t batchNum = freeListSize_[index]; // 需要归还的内存块数量
        if (batchNum <= 1)  return; // 只有1个块则不归还

        // 保留一部分在原空闲链表中（避免频繁向中心缓存申请师范内存） 比如保留个1/4
        size_t keepNum = std::max(batchNum / 4,size_t(1));
        size_t retNum = batchNum - keepNum;
    }
}

