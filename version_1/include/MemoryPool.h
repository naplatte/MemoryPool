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
        // 内存池对象构造，各方面都为0，仅建一个对象
        MemoryPool (size_t BlockSize = 4096) : BlockSize_(BlockSize),SlotSize_(0),
        firstBlock_(nullptr),curSlot(nullptr),freeList_(nullptr),lastSlot_(nullptr){};

        // 析构函数
        ~MemoryPool();

        // 初始化内存池，给出内存池中槽的大小
        void init(size_t size);

        void* allocate();
        void deallocate(void* ptr);

    private:
        void allocateNewBlock();

        // 计算对齐需要填充内存的大小
        size_t padPointer(char* p, size_t slotsize);

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


    // 哈希桶，管理多个内存池
    class HashBucket {
    public:
        // 内存池初始化，建立MEMORY_POOL_NUM个内存池供HashBucket管理
        static void initMemoryPool();

        // 单例，提供内存池对象（实际是个数组）的全局访问点
        static MemoryPool& getMemoryPool(int index);

        static void* useMemory(size_t size);

        static void freeMemory(void* ptr,size_t size);

        // 向内存池申请内存的接口
        template<typename T,typename... Args>
        friend T* newElement(Args&&... args);

        // 将申请的内存进行回收
        template<typename T>
        friend void deleteElement(T* p);
    };


    template<typename T, typename ... Args>
    T * newElement(Args &&...args) {
        T* p = nullptr;
        // 根据元素大小选择合适的内存池分配内存（实际上是根据槽选择合适的内存池）

    }

    template<typename T>
    void deleteElement(T *p) {
    }
}
#endif //MEMORYPOOL_H
