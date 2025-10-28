//
// Created by cuihs on 2025/10/20.
//
#include "MemoryPool.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace Memory_Pool;

// 测试用例
class P1
{
    int id_;
};

class P2
{
    int id_[5];
};

class P3
{
    int id_[10];
};

class P4
{
    int id_[20];
};

// 使用内存池分配释放内存
void BenchMarkMeomoryPool(size_t ntimes,size_t nworks,size_t rounds) { //每轮分配释放次数、线程数、测试轮数
    std::vector<std::thread> vthread(nworks); // 创建nworks个线程的线程池
    size_t total_costtime = 0; // 总耗时
    for (int k = 0; k < nworks; ++k) {

    }
}


// 使用系统new/delete分配释放内存



int main () {
    HashBucket::initMemoryPool();


    return 0;
}