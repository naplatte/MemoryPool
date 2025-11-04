//
// Created by cuihs on 2025/11/4.
//

#ifndef MEMORYPOOL_UTILS_H
#define MEMORYPOOL_UTILS_H

#include <cstddef>
#include <atomic>
#include <array>

namespace MemoryPool {
    constexpr size_t ALIGNMENT = 8; // 最小内存块为8B（内存块需要按8B对齐）
    constexpr size_t MAX_BTTES = 256 * 1024; // 最大内存块25KB，超过则使用new/malloc向os申请
    constexpr size_t FREE_LIST_SIZE = MAX_BTTES / ALIGNMENT; // 空闲槽链表的节点数（最大为256KB / 8B 个）

    // 内存块头部
    struct BlockHeader {
        size_t size; // 内存块大小
        bool isUse; // 是否被使用
        BlockHeader* next; // 指向下一个内存块
    };


}



#endif //MEMORYPOOL_UTILS_H
