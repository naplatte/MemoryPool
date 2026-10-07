#define NOMINMAX
#include <Windows.h>

#include "../include/MemoryPool.h"
#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <cstring>
#include <random>
#include <algorithm>
#include <atomic>

using namespace MemoryPool;

// 基础分配测试
void testBasicAllocation()
{
    std::cout << "正在运行基本分配测试……" << std::endl;

    // 测试小内存分配
    void* ptr1 = MemoryPoolV2::allocate(8);
    assert(ptr1 != nullptr);
    MemoryPoolV2::deallocate(ptr1, 8);

    // 测试中等大小内存分配
    void* ptr2 = MemoryPoolV2::allocate(1024);
    assert(ptr2 != nullptr);
    MemoryPoolV2::deallocate(ptr2, 1024);

    // 测试大内存分配（超过MAX_BYTES）
    void* ptr3 = MemoryPoolV2::allocate(1024 * 1024);
    assert(ptr3 != nullptr);
    MemoryPoolV2::deallocate(ptr3, 1024 * 1024);

    std::cout << "基本分配测试通过！" << std::endl;
}

// 内存写入测试
void testMemoryWriting()
{
    std::cout << "正在运行内存读写测试……" << std::endl;

    // 分配并写入数据
    const size_t size = 128;
    char* ptr = static_cast<char*>(MemoryPoolV2::allocate(size));
    assert(ptr != nullptr);

    // 写入数据
    for (size_t i = 0; i < size; ++i)
    {
        ptr[i] = static_cast<char>(i % 256);
    }

    // 验证数据
    for (size_t i = 0; i < size; ++i)
    {
        assert(ptr[i] == static_cast<char>(i % 256));
    }

    MemoryPoolV2::deallocate(ptr, size);
    std::cout << "内存读写测试通过！" << std::endl;
}

// 多线程测试
void testMultiThreading()
{
    std::cout << "正在运行多线程测试……" << std::endl;

    const int NUM_THREADS = 4;
    constexpr int ALLOCS_PER_THREAD = 1000;
    std::atomic<bool> has_error{false};

    auto threadFunc = [&has_error, ALLOCS_PER_THREAD]()
    {
        try
        {
            std::vector<std::pair<void*, size_t>> allocations;
            allocations.reserve(ALLOCS_PER_THREAD);

            for (int i = 0; i < ALLOCS_PER_THREAD && !has_error; ++i)
            {
                size_t size = (rand() % 256 + 1) * 8;
                void* ptr = MemoryPoolV2::allocate(size);

                if (!ptr)
                {
                    std::cerr << "内存分配失败，申请大小：" << size << std::endl;
                    has_error = true;
                    break;
                }

                allocations.push_back({ptr, size});

                if (rand() % 2 && !allocations.empty())
                {
                    size_t index = rand() % allocations.size();
                    MemoryPoolV2::deallocate(allocations[index].first,
                                           allocations[index].second);
                    allocations.erase(allocations.begin() + index);
                }
            }

            for (const auto& alloc : allocations)
            {
                MemoryPoolV2::deallocate(alloc.first, alloc.second);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "工作线程发生异常：" << e.what() << std::endl;
            has_error = true;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(threadFunc);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    std::cout << "多线程测试通过！" << std::endl;
}

// 边界测试
void testEdgeCases()
{
    std::cout << "正在运行边界测试……" << std::endl;

    // 测试0大小分配
    void* ptr1 = MemoryPoolV2::allocate(0);
    assert(ptr1 != nullptr);
    MemoryPoolV2::deallocate(ptr1, 0);

    // 测试最小对齐大小
    void* ptr2 = MemoryPoolV2::allocate(1);
    assert(ptr2 != nullptr);
    assert((reinterpret_cast<uintptr_t>(ptr2) & (ALIGNMENT - 1)) == 0);
    MemoryPoolV2::deallocate(ptr2, 1);

    // 测试最大大小边界
    void* ptr3 = MemoryPoolV2::allocate(MAX_BYTES);
    assert(ptr3 != nullptr);
    MemoryPoolV2::deallocate(ptr3, MAX_BYTES);

    // 测试超过最大大小
    void* ptr4 = MemoryPoolV2::allocate(MAX_BYTES + 1);
    assert(ptr4 != nullptr);
    MemoryPoolV2::deallocate(ptr4, MAX_BYTES + 1);

    std::cout << "边界测试通过！" << std::endl;
}

// 压力测试
void testStress()
{
    std::cout << "正在运行压力测试……" << std::endl;

    const int NUM_ITERATIONS = 10000;
    std::vector<std::pair<void*, size_t>> allocations;
    allocations.reserve(NUM_ITERATIONS);

    for (int i = 0; i < NUM_ITERATIONS; ++i)
    {
        size_t size = (rand() % 1024 + 1) * 8;
        void* ptr = MemoryPoolV2::allocate(size);
        assert(ptr != nullptr);
        allocations.push_back({ptr, size});
    }

    // 随机顺序释放
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(allocations.begin(), allocations.end(), g);
    for (const auto& alloc : allocations)
    {
        MemoryPoolV2::deallocate(alloc.first, alloc.second);
    }

    std::cout << "压力测试通过！" << std::endl;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    try
    {
        std::cout << "开始运行内存池单元测试……" << std::endl;

        testBasicAllocation();
        testMemoryWriting();
        testMultiThreading();
        testEdgeCases();
        testStress();

        std::cout << "全部单元测试通过！" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "单元测试失败：" << e.what() << std::endl;
        return 1;
    }
}
