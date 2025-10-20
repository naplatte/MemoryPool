//
// Created by cuihs on 2025/10/19.
//
#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <atomic>
#include <mutex>

namespace Memory_Pool {
#define MEMORY_POOL_NUM 64 // 内存池中槽（子池）的数量
#define SLOT_BASE_SIZE 8 // 每个槽之间的大小递增数量（最小分配单元）
#define MAX_SLOT_SIZE 512 // 最大槽的可分配内存大小

    struct Slot {
        std::atomic<Slot*> next; // next对象，对Slot指针类型可进行原子访问
    };

    class MemoryPool {
    public:
        MemoryPool (size_t BlockSize = 4096) : BlockSize_(BlockSize){};
        ~MemoryPool();

        void init(size_t size);

        void* allocate();
        void deallocate(void* ptr);

    private:
        void allocateNewBlock();
        size_t padPointer(char* p, size_t align);

        // 使用CAS进行无锁入队和出队
        bool pushFreeList(Slot* slot);
        Slot* popFreeList();

    private:
        int BlockSize_; // 内存块大小
        int SlotSize_; // 槽大小
        Slot* firstBlock_; // 指向内存池管理的首个内存块
        Slot* curSlot; // 指向当前未被使用过的槽（将要分配出去的下一个槽）
        std::atomic<Slot*> freeList_; // 指向空闲的槽（被使用后又被释放）
        Slot* lastSlot_; // 当前内存块中最后能够存放元素的位置标识（超过该位置需要申请新内存块）
        std::mutex mutexForBlock_;
    };
}

// 哈希桶，管理多规格内存池
class HashBucket {

};

#endif //MEMORYPOOL_H
