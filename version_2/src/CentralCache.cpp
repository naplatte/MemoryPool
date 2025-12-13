#include "../include/CentralCache.h"
#include "../include/PageCache.h"

namespace MemoryPool
{
static const size_t SPANPAGES = 8; // 每次从页缓存获取的span大小，小需求固定拿 8 页，大需求按实际需要的页数拿
const std::chrono::milliseconds CentralCache::DELAY_INTERVAL{1000};


CentralCache::CentralCache() {
    // 所有大小的内存块对应的空闲链表，初始为空 memory_order_relaxed - 不保证顺序，仅保证原子性
    for (auto& ptr : centralFreeList_) {
        ptr.store(nullptr,std::memory_order_relaxed);
    }

}

void *CentralCache::fetchRange(size_t index) {
    if (index >= FREE_LIST_SIZE)
        return nullptr;
    // 自旋锁 - test and set
    while (locks_[index].test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield(); // 线程让出CPU，避免忙等占用资源
    }

    // 退出while说明获得了所需大小内存块对应空闲链表的使用权
    void* res = nullptr;
    try {
        res = centralFreeList_[index].load(std::memory_order_relaxed);

        // 中心缓存为空
        if (! res) {
            // 向页缓存申请
            size_t size = (index + 1) * ALIGNMENT;
            res = fetchFromPageCache(size);

            // 页缓存也为空
            if (! res) {
                locks_[index].clear(std::memory_order_release);
                return nullptr;
            }
            // 将获取到的内存块分为小块（从页缓存申请来的是页区间）
            char* start = static_cast<char*>(res);

            // 实际分配到的页数 - 小于8页的需求分8页，大于8页的需求按实际去分
            size_t numPages = (size <= SPANPAGES * PageCache::PAGE_SIZE) ?
                              SPANPAGES :
                              (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
            // 计算块数
            size_t blockNum = (numPages * PageCache::PAGE_SIZE) / size;

            if (blockNum > 1) {
                // 两个块才能构建空闲链表
                for (auto i = 1; i < blockNum; ++i) {
                    void* cur = start + (i - 1) * size;
                    void* next = start + i * size;
                    *reinterpret_cast<void**>(cur) = next;
                }
                *reinterpret_cast<void**>(start + (blockNum - 1) * size) = nullptr;

                // 保存res的下一个节点
                void* next = *reinterpret_cast<void**>(res);
                *reinterpret_cast<void**>(res) = nullptr;

                // 更新中心缓存
                centralFreeList_[index].store(next,std::memory_order_release);

                // 无锁方式记录span信息，为归还内存到页缓存考虑
                size_t trackerIndex = spanCount_++;
                if (trackerIndex < spanTracks_.size()) { // 若追踪对象已满（最高1024个），则直接不记录 - 静默丢弃
                    spanTracks_[trackerIndex].spanAddr.store(start,std::memory_order_release);
                    spanTracks_[trackerIndex].numPages.store(numPages,std::memory_order_release);
                    spanTracks_[trackerIndex].blockCount.store(blockNum,std::memory_order_release);
                    spanTracks_[trackerIndex].freeCount.store(blockNum - 1,std::memory_order_release); // 第一个块res已被分配
                }
            }
         }

        // 从中心缓存成功拿到所需大小的内存块
        else {
            void* next = *reinterpret_cast<void**>(res);
            *reinterpret_cast<void**>(res) = nullptr;
            centralFreeList_[index].store(next,std::memory_order_release);

            // 更新span空闲计数
            SpanTracker* tracker = getSpanTracker(res);
            if (tracker) {
                // 减少一个空闲块
                tracker->freeCount.fetch_sub(1,std::memory_order_release);
            }
        }
    }
    catch (...) {
        locks_[index].clear(std::memory_order_release);
        throw ;
    }

    //释放锁
    locks_[index].clear(std::memory_order_release);

    return res;
}

void CentralCache::returnRange(void *start, size_t size, size_t index) {
    if (!start || index >= FREE_LIST_SIZE)
        return;
    size_t blockSize = (index + 1) * ALIGNMENT;
    size_t blockCount = size / blockSize;

    while (locks_[index].test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    try {
        // 归还的空闲链表 链接到 中心缓存的空闲链表
        void* end = start; // 当前归还的块的地址
        size_t count = 1; // 剩余空闲块数
        while (*reinterpret_cast<void**>(end) != nullptr && blockCount > count) {
            end = *reinterpret_cast<void**>(end); // next指针
            count++;
        }
        void* cur = centralFreeList_[index].load(std::memory_order_relaxed);
        *reinterpret_cast<void**>(end) = cur; // 原空闲链表 链接在 归还链表后面
        centralFreeList_[index].store(start,std::memory_order_release);

        // 更新延迟计数
        size_t curCount = delayCounts_[index].fetch_add(1,std::memory_order_relaxed) + 1;
        auto curTime = std::chrono::steady_clock::now();

        // 检查是否需延迟归还
        if (shouldPerformDelayedReturn(index,curCount,curTime)) {
            performDelayedReturn(index);
        }
    }
    catch (...) {
        locks_[index].clear(std::memory_order_release);
        throw;
    }
    // 解锁
    locks_[index].clear(std::memory_order_release);
}

void* CentralCache::fetchFromPageCache(size_t size) {
    // 计算需要分配的页数
    size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;

    // 根据大小决定分配策略
    if (size <= SPANPAGES * PageCache::PAGE_SIZE) {
        // 页->中心，至少是8页
        return PageCache::getInstance().allocateSpan(SPANPAGES);
    }
    else {
        return PageCache::getInstance().allocateSpan(numPages);
    }
}

SpanTracker *CentralCache::getSpanTracker(void *blockAddr) {
    // 遍历存储Spantrackers的数组，找到块地址所属的spantracker
    for (size_t i = 0; i < spanCount_.load(std::memory_order_relaxed); ++i) {
        void* spanAddr = spanTracks_[i].spanAddr.load(std::memory_order_relaxed);
        size_t numPages = spanTracks_[i].numPages.load(std::memory_order_relaxed);

        if (blockAddr >= spanAddr && blockAddr < static_cast<char*>(spanAddr) + numPages * PageCache::PAGE_SIZE) {
            return &spanTracks_[i];
        }
    }
    return nullptr;
}

// 之后再补这里
void CentralCache::updateSpanFreeCount(SpanTracker *tracker, size_t newFreeBlocks, size_t index) {

}

bool CentralCache::shouldPerformDelayedReturn(size_t index, size_t currentCount,std::chrono::steady_clock::time_point currentTime) {
    if (currentCount >= MAX_DELAY_COUNT) {
        return true;
    }

    auto lastTime = lastReturnTimes_[index];
    return (currentTime - lastTime) >= DELAY_INTERVAL;
}

void CentralCache::performDelayedReturn(size_t index) {
    // 重置延迟计数
    delayCounts_[index].store(0,std::memory_order_relaxed);
    // 更新最后归还时间
    lastReturnTimes_[index] = std::chrono::steady_clock::now();

    // 统计每个大小内存块对应span（页区间）的空闲块数
    std::unordered_map<SpanTracker*,size_t > spanFreeCounts;
    void* curBlock = centralFreeList_[index].load(std::memory_order_relaxed); // 遍历空闲链表中空闲块的指针

    while (curBlock) {
        SpanTracker* tracker = getSpanTracker(curBlock);
        if (tracker) {
            spanFreeCounts[tracker]++;
        }
        curBlock = *reinterpret_cast<void**>(curBlock);
    }

    // 更新每个span的空闲计数并检查是否可归还
    for (const auto& [tracker, newFreeBlocks] : spanFreeCounts)
    {
        updateSpanFreeCount(tracker, newFreeBlocks, index);
    }

}


} // MemoryPool