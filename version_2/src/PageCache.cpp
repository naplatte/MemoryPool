#include "../include/PageCache.h"
#include <Windows.h>

void *PageCache::allocateSpan(size_t numPages) {
    // RAII 现在加锁，等mutex_作用域结束后，自动解锁该信号量
    // 即进入函数时，上锁mutex_，退出函数时解锁，保证多线程操作freeSpans_和spanMap_的安全性
    std::lock_guard<std::mutex> lock(mutex_);

    // 找到第一个合适的Span（页数>=numPages）并分配
    auto it = freeSpans_.lower_bound(numPages); // lower_bound:返回指向第一个 key 不小于给定值（>= key）的元素的迭代器
    if (it != freeSpans_.end()) {
        Span* span = it->second;
        // 将取出的span从原空闲页区间链表中移除
        if (span->next) {
            freeSpans_[it->first] = span->next; // 即删除页数为it->first大小的空闲页区间的第一个节点
        }
        else { // 当前空闲页区间只有1个元素
            freeSpans_.erase(it);
        }

        // 若span页数 > numPages则需要进行分割，将剩余部分(newspan)放回空闲页区间链表
        if (span->numPages > numPages) {
            Span* newSpan = new Span; // 创建新的大小的空闲页区间
            newSpan->pageAddr = static_cast<char*>(span->pageAddr) + numPages * PAGE_SIZE;
            newSpan->numPages = span->numPages - numPages;
            newSpan->next = nullptr;

            auto& node = freeSpans_[newSpan->numPages];
            newSpan->next = node;
            node = newSpan; // 头插

            span->numPages = numPages;
        }

        // 记录span信息用于回收
        spanMap_[span->pageAddr] = span;
        return span->pageAddr;
    }
    // 空闲页区间内没有合适的span，则向系统申请
    void* memory = systemAlloc(numPages);
    if (! memory)
        return nullptr;

    // 创建新的span记录从系统申请来的span
    Span* span = new Span;
    span->pageAddr = memory;
    span->numPages = numPages;
    span->next = nullptr;
    // 记录span信息用于回收
    spanMap_[memory] = span;
    return memory;
}

void PageCache::deallocateSpan(void *ptr, size_t numPages) {

}

void *PageCache::systemAlloc(size_t numPages) {
    size_t size = numPages * PAGE_SIZE; // 需要内存总量

    // Windows下使用VirtualAlloc 分配内存
    void* ptr = VirtualAlloc(nullptr,size,MEM_RESERVE | MEM_COMMIT,PAGE_READWRITE);
    if (! ptr)
        return nullptr;

    memset(ptr,0,size); // 刚分配到的那一段内存里的每一个字节都写成 0，相当于做一次初始化 memset用于批量填充内存
    return ptr;

}
