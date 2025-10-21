//
// Created by cuihs on 2025/10/19.
//

#include "MemoryPool.h"

namespace Memory_Pool {
    MemoryPool::~MemoryPool() {
    }

    void MemoryPool::init(size_t size) {
    }

    void * MemoryPool::allocate() {
    }

    void MemoryPool::deallocate(void *ptr) {
    }

    void MemoryPool::allocateNewBlock() {
    }

    size_t MemoryPool::padPointer(char *p, size_t align) {
    }

    bool MemoryPool::pushFreeList(Slot *slot) {
    }

    Slot * MemoryPool::popFreeList() {
    }

    void HashBucket::initMemoryPool() {
    }

}

