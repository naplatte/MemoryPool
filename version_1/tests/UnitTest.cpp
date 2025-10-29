//
// Created by cuihs on 2025/10/20.
//
#include "MemoryPool.h"
#include <iostream>
#include <thread>
#include <valarray>
#include <vector>
#include <windows.h>

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
void BenchMarkMemoryPool(size_t ntimes,size_t nworks,size_t rounds) { //每轮分配释放次数、线程数、测试轮数
    std::vector<std::thread> vthread(nworks); // 创建nworks个线程的线程池
    size_t total_costtime = 0; // 总耗时
    for (int k = 0; k < nworks; ++k) {
        vthread[k] = std::thread([&]() {
           for (int j = 0; j < rounds; ++j) {
               int begin1 = clock(); // 从程序启动到当前时刻所使用的 CPU 时间
               for (int i = 0; i < ntimes; i++) {
                   P1* p1 = newElement<P1>();
                   deleteElement<P1>(p1);
                   P2* p2 = newElement<P2>();
                   deleteElement<P2>(p2);
                   P3* p3 = newElement<P3>();
                   deleteElement<P3>(p3);
                   P4* p4 = newElement<P4>();
                   deleteElement<P4>(p4);
               }
               int end1 = clock();

               total_costtime =  total_costtime + (end1 - begin1);
           }
        });
    }
    for (auto& t : vthread) {
        t.join();
    }
    printf("%lu个线程并发执行%lu轮次，每轮次newElement&deleteElement %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);

}


// 使用系统new/delete分配释放内存

void BenchMarkNew(size_t ntimes,size_t nworks,size_t rounds) {
    std::vector<std::thread> vthread(nworks);
    int total_costtime = 0;
    for (int i = 0; i < nworks; ++i) {
        vthread[i] = std::thread([&]() {
           for (int j = 0; j < rounds; ++j) {
               int begin1 = clock();
               for (int k = 0; k < ntimes; ++k) {
                   P1* p1 = new P1;
                   delete p1;
                   P2* p2 = new P2;
                   delete p2;
                   P3* p3 = new P3;
                   delete p3;
                   P4* p4 = new P4;
                   delete p4;
               }
               int end1 = clock();
               total_costtime = total_costtime + (end1 - begin1);
           }
        });
    }
    for (auto& i : vthread) {
        i.join();
    }
	printf("%lu个线程并发执行%lu轮次，每轮次malloc&free %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);

}




int main () {
    SetConsoleOutputCP(CP_UTF8); // 设置输出为 UTF-8
    SetConsoleCP(CP_UTF8);       // 设置输入为 UTF-8

    HashBucket::initMemoryPool();
	BenchMarkMemoryPool(100, 50, 10); // 测试内存池
    std::cout << "===========================================================================" << std::endl;
    BenchMarkNew(100, 50, 10); // 测试 new delete

    return 0;
}