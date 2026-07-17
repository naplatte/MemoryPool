- 三级缓存实现：设计 ThreadCache、CentralCache 与 PageCache 分层结构，按照 8B 对⻬划分不同⼤⼩类；⼩对象
优先从线程本地空闲链表分配，超过 256KB 的⼤对象直接向系统申请；
- 并发与回收机制：通过线程局部存储实现线程独⽴缓存；CentralCache 按⼤⼩类设置⾃旋锁并批量获取、归还内存，
PageCache 基于 VirtualAlloc 按⻚申请内存并进⾏ Span 分割与回收；
- 测试验证：编写单元、边界、压⼒及性能测试，覆盖单线程、多线程和混合⼤⼩对象的分配释放，并与 C++ 原⽣
new/delete 进⾏耗时对⽐。
