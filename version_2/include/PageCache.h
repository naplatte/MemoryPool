#ifndef MEMORYPOOL_PAGECACHE_H
#define MEMORYPOOL_PAGECACHE_H
#include "utils.h"
#include <map>
#include <mutex>

class PageCache {
public:
    static const size_t PAGE_SIZE = 4096; // 4KB - 页大小

    static PageCache& getInstance() {
        static PageCache instance;
        return instance;
    }

private:
    PageCache() = default; // 构造函数私有实现单例

    // 向系统申请内存
    void* systemAlloc(size_t numPages);

private:
    // 一段连续的页区间
    struct Span {
        void* pageAddr; // 页起始地址
        size_t numPages; // 页数
        Span* next; // 指向下一段页区间的指针
    };

    std::map<size_t,Span*> freeSpans_; // 不同页数的Span对应不同的空闲链表
    std::map<void*,Span*> spanMap_;// 根据页地址 找对对应的Span（区间）
    std::mutex mutex_;
};


#endif //MEMORYPOOL_PAGECACHE_H
