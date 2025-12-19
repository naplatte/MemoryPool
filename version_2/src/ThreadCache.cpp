#include <cstdlib>
#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"

namespace MemoryPool
{
// 分配相关逻辑
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

    void *ThreadCache::fetchFromCentralCache(size_t index) {
        // 调用中心缓存取内存块的接口，获得指向内存块的指针
        void* start = CentralCache::getInstance().fetchRange(index);
        // 中心缓存未能提供内存块,返回空指针
        if (! start)
            return nullptr;

        // 第一个块返回给调用者
        void* res = start;

        // 获取剩余的块（如果有的话）
        void* next = *reinterpret_cast<void**>(start);

        // 如果有剩余的块，统计数量并加入空闲链表
        if (next != nullptr) {
            // 将剩余块加入空闲链表
            freeList_[index] = next;

            // 统计剩余块的数量
            size_t num = 0;
            void* cur = next;
            while (cur != nullptr) {
                num++;
                cur = *reinterpret_cast<void**>(cur);
            }
            freeListSize_[index] += num;
        }

        // 清空返回块的next指针，避免调用者误用
        *reinterpret_cast<void**>(res) = nullptr;

        return res;
    }


// 归还相关逻辑
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

    bool ThreadCache::shouldReturnToCentralCache(size_t index) {
        // 设定阈值，当对应大小的内存块空闲链表的空闲块数量大于阈值时，将内存回收
        size_t threshold = 256;
        return (freeListSize_[index] > threshold);
    }

    void ThreadCache::returnToCentralCache(void *start, size_t size) {
        size_t index = SizeClass::getIndex(size);
        // 获取对其后的实际内存大小
        size_t alignedSize = SizeClass::roundUp(size);

        // 计算归还的内存数量
        size_t batchNum = freeListSize_[index];
        if (batchNum <= 1)
            return;

        // 保留一部分在线程缓存（不还干净，免得下次再借），这里以留1/4为例
        size_t keepNum = std::max(batchNum / 4,size_t(1));
        size_t retNum = batchNum - keepNum;

        // 将内存块（包括留着的）串成链表
        char* cur = static_cast<char*>(start);
        char* splitNode = cur;
        for (size_t i = 0; i < keepNum - 1; ++i) {
            splitNode = reinterpret_cast<char*>(*reinterpret_cast<void**>(splitNode));
            // 防御性处理“空闲链表长度不符合预期”的情况，避免解引用空指针，同时修正实际需要归还的内存块数量
            if (splitNode == nullptr) {
                retNum = batchNum - (i + 1); // 链表比预期的短，则归还链表中存在的所有块
                break;
            }
        }
        if (splitNode != nullptr) {
            // 划分归还 & 保留
            void* nextNode = *reinterpret_cast<void**>(splitNode);
            *reinterpret_cast<void**>(splitNode) = nullptr; // 断链

            // 更新线程缓存的空闲链表
            freeList_[index] = start; // 头插
            freeListSize_[index] = keepNum;

            // 归还给中心链表
            if (retNum > 0 && nextNode != nullptr) {
                CentralCache::getInstance().returnRange(nextNode,retNum * alignedSize,index);
            }
        }
    }
}

