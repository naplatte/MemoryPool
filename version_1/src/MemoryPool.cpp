//
// Created by cuihs on 2025/10/19.
//

#include "MemoryPool.h"

#include <cassert>

namespace Memory_Pool {
    MemoryPool::~MemoryPool() {
        // 释放内存池中连续内存块的空间
        Slot* cur = firstBlock_;
        while (cur) {
            Slot* next = cur->next;
            // 将指向块的指针转为void*类型 - 因为void*类型不需要调用析构函数，只释放空间
            // 等同于 free(reinterpret_cast<void*>(firstBlock_));
            operator delete(reinterpret_cast<void*>(cur));
            cur = next;
        }
    }

    void MemoryPool::init(size_t size) {
        assert(size > 0);
        SlotSize_ = size;
        firstBlock_ = nullptr;
        curSlot = nullptr;
        freeList_ = nullptr;
        lastSlot_ = nullptr;
    }

    void * MemoryPool::allocate() {
        // 优先使用freelist中的内存槽
        Slot* slot = popFreeList();
        if (slot != nullptr) {
            return slot;
        }
        // freelist为空 - 使用curslot指向的槽
        Slot* temp;
        {
            std::lock_guard<std::mutex> lock(mutexForBlock_);
            if (curSlot >= lastSlot_) {
                // 当前内存块已满，向OS申请新内存块
                allocateNewBlock();
            }
            temp = curSlot;
            // 将curSlot从当前位置移动到下一个内存槽的起始地址
            // 不能直接curSlot + Slotsize_，因为这样会便宜Slotsize_ * sizeof(Slot)个字节
            curSlot += (SlotSize_ / sizeof(Slot));
        }
        return temp;
    }

    void MemoryPool::deallocate(void *ptr) {
        if (! ptr)
            return;
        // 放入freelist中
        Slot* slot = reinterpret_cast<Slot *>(ptr);
        pushFreeList(slot);
    }

    void MemoryPool::allocateNewBlock() {

    }

    size_t MemoryPool::padPointer(char *p, size_t align) {
    }

    // 实现无锁入队
    bool MemoryPool::pushFreeList(Slot *slot) {
        while (true) {
            // 获取当前头结点 (freeList_ 本身存的就是链表头的地址,直接load就可以得到)
            Slot* oldHead = freeList_.load(std::memory_order_relaxed);
            // 新节点头插(slot->next = head)
            slot->next.store(oldHead,std::memory_order_relaxed);
            // 新节点设为头结点
            if (freeList_.compare_exchange_weak(oldHead,slot,
                std::memory_order_release,std::memory_order_relaxed));
                return true;
            // 如果失败的话说明另一个线程可能修改了freelist_
        }
    }

    // 实现无锁出队
    Slot * MemoryPool::popFreeList() {
        while (true) {
            Slot* oldHead = freeList_.load(std::memory_order_acquire);
            // 头删
            if (oldHead == nullptr)
                return nullptr; // 队列为空
            Slot* newHead = nullptr;
            try {
                newHead = oldHead->next.load(std::memory_order_relaxed); // 头节点的next成为新节点（删头）
            }
            catch (...) {
                // 如果捕获到异常，则回到while开始的地方重试
                continue;
            }

            // 再检查一遍newHead是否被真的改了，没改再改，改了直接return （多线程必须要考虑这些）
            if (freeList_.compare_exchange_weak(oldHead,newHead,
                std::memory_order_acquire,std::memory_order_relaxed))
            {
                return oldHead;
            }
            // 失败：说明另一个线程可能已经修改了 freeList_
        }
    }

    void HashBucket::initMemoryPool() {
        for (int i = 0; i < MEMORY_POOL_NUM; ++i) {
            // index 暗含槽大小信息
            getMemoryPool(i).init((i+1) * SLOT_BASE_SIZE);
        }
    }

    MemoryPool & HashBucket::getMemoryPool(int index) {
        static MemoryPool memorypool[MEMORY_POOL_NUM]; // 通过静态数组创建了多个MemoryPool实例 单例
        return memorypool[index];
    }

    void * HashBucket::useMemory(size_t size) {
        if (size <= 0)
            return nullptr;
        // 超过最大槽限制，则使用operator new 分配内存
        if (size > MAX_SLOT_SIZE)
            return operator new(size);
        // size/8 向上取整
        return getMemoryPool((size + 7) / SLOT_BASE_SIZE).allocate();
    }

    void HashBucket::freeMemory(void *ptr, size_t size) {
        if (!ptr)
            return;
        if (size > MAX_SLOT_SIZE) {
            operator delete(ptr);
            return;
        }
        getMemoryPool((size + 7) / SLOT_BASE_SIZE - 1).deallocate(ptr);
    }
}

