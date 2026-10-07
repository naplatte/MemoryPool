# C++ 三级缓存并发内存池

项目包含 ThreadCache、CentralCache 与 PageCache 的实现及测试代码。

## 目录结构

- `include/`：头文件与内存池接口。
- `src/`：三级缓存实现。
- `tests/`：单元测试与性能测试。
- `cmake/CTestCustom.cmake`：Windows 下的 CTest 日志时间配置。
- `CMakeLists.txt`：构建配置。

## Windows 下构建与测试

在 PowerShell 中使用 CMake 和 MinGW：

```powershell
cd D:\Code\MemoryPool
$env:Path = "D:\soft\msys\ucrt64\bin;$env:Path"

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

单元测试在 Release 构建中也保留断言检查。测试程序以 UTF-8 输出中文。性能测试还会在程序所在目录生成独立的中文日志 `build/性能测试.log`；包含 CTest 运行记录的完整日志保存在 `build/Testing/Temporary/LastTest.log`。

CTest 在 Windows 下可能将中文时区名按系统本地编码写入日志，导致 UTF-8 编辑器显示乱码。配置阶段会自动生成 `build/CTestCustom.cmake`，使 CTest 使用英文时区名 `CST`，保留北京时间（UTC+8）。重新配置后正常运行 `ctest` 即可，无需修改 Windows 的语言或时区设置。

## 性能测试方法

覆盖小对象（8～256 B）和混合大小（8 B～4 KB）两类负载，分别使用 1、2、4、8 个线程。混合大小的每次分配按 60%／30%／10% 的概率选择小、中、大三组；这些分组不分别对应三级缓存。

每个线程数均执行相同的总工作量：默认每轮共 20 万次分配和 20 万次释放，总存活对象上限为 2048 个，在线程之间分摊。

- 提前生成固定种子的分配／释放序列，两种分配器逐项重放相同操作；程序核对序列摘要和分配次数。
- 每次测量使用新的独立进程，避免前一轮的内存池状态影响后一轮。
- “首次使用”（内部参数 `first-use`）：不进行显式分配器预热，计时包含首次使用时的初始化。准备工作自身会使用运行库分配内存，因此该模式不表示整个系统堆从未被使用。
- “预热后”（内部参数 `warm`）：每个工作线程在计时前完整重放一次同样的负载，为内存池和 new/delete 提供相同预热条件。
- 操作序列生成、线程创建、预热和线程退出不计时；计时包括分配、释放、槽位管理以及相同的首尾字节写入和校验。
- 默认每种场景测量 5 轮，交替两种分配器的执行顺序，输出耗时中位数、最小值和最大值。
- “耗时比（内存池 / new/delete）”是两种分配器耗时中位数的比值，小于 1 表示该负载下内存池更快。

因此输出是带少量共同管理、同步及读写检查的负载耗时，不是单次分配调用的延迟，也不能直接说明生产环境中的性能优势。测试不包含跨线程释放、4 KB 以上的对象以及进程内存占用统计。

直接运行性能测试：

```powershell
# 默认 5 轮，每轮总计 20 万次分配。
.\build\MemoryPool_PerformanceTest.exe

# 完整覆盖全部场景的短轮检查，不能作为正式性能结论。
.\build\MemoryPool_PerformanceTest.exe --quick

# 增加轮数与样本量；线程数增加时，总工作量仍保持一致。
.\build\MemoryPool_PerformanceTest.exe --rounds 7 --allocations 1000000
```

性能测试遇到分配失败、数据校验失败、子进程异常或序列不一致时会以非零状态退出。`build/` 为构建产物目录，可以删除后重新生成。
