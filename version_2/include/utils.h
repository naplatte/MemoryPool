//
// Created by cuihs on 2025/11/4.
//

#ifndef MEMORYPOOL_UTILS_H
#define MEMORYPOOL_UTILS_H

#include <cstddef>
#include <atomic>
#include <array>

namespace MemoryPool
{
constexpr size_t ALIGNMENT = 8; // 最小内存块为8B（内存块需要按8B对齐）
constexpr size_t MAX_BTTES = 256 * 1024; // 最大内存块256KB，超过则使用new/malloc向os申请
constexpr size_t FREE_LIST_SIZE = MAX_BTTES / ALIGNMENT; // 空闲槽链表的节点数（最大为256KB / 8B 个）

// 内存块头部
struct BlockHeader {
    size_t size; // 内存块大小
    bool isUse; // 是否被使用
    BlockHeader* next; // 指向下一个内存块
};

// 内存块の标准化大小和索引映射
class SizeClass {
public:
    // 将 bytes 向上取整为 ALIGNMENT 的倍数
    static size_t roundUp(size_t bypes) {
        // 公式原理:“多加一点，然后整除，再乘回去”，减1是因为，如果是8B这样的，8+8就是16，就会被扩到16B的块（但实际只需要8B）
        // & 是按位与操作
        // 以ALIGNMENT = 8为例，ALIGNMENT - 1 = 7,二进制为0111,经 ~ 按位取反后就是1000
        // 因此按位与操作就相当于将(bypes + ALIGNMENT - 1)的后三位清0
        // 比如原size = 7B，(7+8-1) = 14.二进制为0000 1110，与7的取反(1111 1000)按位与后，即0000 1110 & 1111 1000 = 1000，即为8B
        // 与1000，相当于 / 8
        // 位运算版本更高效（编译器级别优化更好）
        return (bypes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    // 将请求映射到对应的空闲链表下标
    static size_t getIndex(size_t bytes) {
        // 确保bytes至少为ALIGNMENT
        bytes = std::max(bytes,ALIGNMENT);

        return ((bytes + ALIGNMENT - 1) / ALIGNMENT) - 1; // 下标从0开始
    }
};


}



#endif //MEMORYPOOL_UTILS_H
