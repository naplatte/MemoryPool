# C++ 三级缓存并发内存池

项目包含 ThreadCache、CentralCache 与 PageCache 的实现及测试代码。

## 目录结构

- `include/`：头文件与内存池接口。
- `src/`：三级缓存实现。
- `tests/`：单元测试与性能测试。
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

`build/` 由构建命令生成，可以随时删除后重新生成。
