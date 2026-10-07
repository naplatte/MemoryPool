#define NOMINMAX
#include <Windows.h>

#include "../include/MemoryPool.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
constexpr size_t TOTAL_LIVE_SLOTS = 2048;
constexpr uint32_t SEED = 20261007;

struct Operation {
    size_t slot;
    size_t size;
    unsigned char marker;
    bool allocate;
};
struct Workload {
    std::vector<Operation> operations;
    size_t allocations = 0;
    size_t slotCount = 0;
    uint64_t digest = 14695981039346656037ULL;
};
struct Slot {
    void* pointer = nullptr;
    size_t size = 0;
    unsigned char marker = 0;
};
struct Sample {
    double milliseconds = 0;
    size_t allocations = 0;
    uint64_t digest = 0;
};
struct Config {
    std::string allocator;
    std::string workload;
    std::string phase;
    size_t threads;
    size_t allocations;
};

void hashValue(uint64_t& digest, uint64_t value) {
    // Hash fixed-width fields, not struct padding or process-specific pointers.
    for (int byte = 0; byte < 8; ++byte) {
        digest ^= (value >> (byte * 8)) & 0xff;
        digest *= 1099511628211ULL;
    }
}

Workload makeWorkload(const std::string& kind, size_t allocations, size_t worker, size_t threads) {
    const std::array<size_t, 6> small{8, 16, 32, 64, 128, 256};
    const std::array<size_t, 5> mixedSmall{8, 16, 32, 64, 128};
    const std::array<size_t, 3> medium{256, 384, 512};
    const std::array<size_t, 3> large{1024, 2048, 4096};
    std::mt19937 random(SEED + static_cast<uint32_t>(worker * 7919));
    Workload result;
    result.allocations = allocations;
    result.slotCount = TOTAL_LIVE_SLOTS / threads + (worker < TOTAL_LIVE_SLOTS % threads ? 1 : 0);
    result.operations.reserve(allocations * 2);
    std::vector<size_t> freeSlots(result.slotCount), liveSlots;
    std::iota(freeSlots.begin(), freeSlots.end(), 0);
    liveSlots.reserve(result.slotCount);
    std::vector<Slot> metadata(result.slotCount);
    auto record = [&](size_t slot, bool allocate) {
        const auto& entry = metadata[slot];
        result.operations.push_back({slot, entry.size, entry.marker, allocate});
        hashValue(result.digest, slot);
        hashValue(result.digest, entry.size);
        hashValue(result.digest, entry.marker);
        hashValue(result.digest, allocate);
    };
    auto releaseOne = [&] {
        const size_t index = random() % liveSlots.size();
        const size_t slot = liveSlots[index];
        record(slot, false);
        liveSlots[index] = liveSlots.back();
        liveSlots.pop_back();
        freeSlots.push_back(slot);
    };
    size_t allocated = 0;
    while (allocated < allocations) {
        const bool allocate = liveSlots.empty() ||
            (!freeSlots.empty() && random() % 100 < 60);
        if (!allocate) {
            releaseOne();
            continue;
        }
        const size_t index = random() % freeSlots.size();
        const size_t slot = freeSlots[index];
        freeSlots[index] = freeSlots.back();
        freeSlots.pop_back();
        size_t size = 0;
        if (kind == "small") {
            size = small[random() % small.size()];
        } else {
            const auto category = random() % 100;
            if (category < 60) size = mixedSmall[random() % mixedSmall.size()];
            else if (category < 90) size = medium[random() % medium.size()];
            else size = large[random() % large.size()];
        }
        metadata[slot].size = size;
        metadata[slot].marker = static_cast<unsigned char>(1 + random() % 127);
        record(slot, true);
        liveSlots.push_back(slot);
        ++allocated;
    }
    while (!liveSlots.empty()) releaseOne();
    if (result.operations.size() != allocations * 2) {
        throw std::runtime_error("分配与释放操作数量不匹配");
    }
    return result;
}

template<bool UsePool>
void replay(const Workload& workload, std::vector<Slot>& slots) {
    for (const auto& operation : workload.operations) {
        auto& slot = slots[operation.slot];
        if (operation.allocate) {
            if (slot.pointer) throw std::runtime_error("尝试向仍在使用的槽位重复分配内存");
            if constexpr (UsePool) {
                slot.pointer = MemoryPool::MemoryPoolV2::allocate(operation.size);
            } else {
                slot.pointer = new char[operation.size];
            }
            if (!slot.pointer) throw std::bad_alloc();
            slot.size = operation.size;
            slot.marker = operation.marker;
            auto* bytes = static_cast<volatile unsigned char*>(slot.pointer);
            bytes[0] = slot.marker;
            bytes[slot.size - 1] = static_cast<unsigned char>(slot.marker ^ 0xff);
        } else {
            if (!slot.pointer || slot.size != operation.size) {
                throw std::runtime_error("释放操作与已分配内存不匹配");
            }
            auto* bytes = static_cast<volatile unsigned char*>(slot.pointer);
            if (bytes[0] != operation.marker ||
                bytes[slot.size - 1] != static_cast<unsigned char>(operation.marker ^ 0xff)) {
                throw std::runtime_error("仍在使用的内存数据被覆盖");
            }
            if constexpr (UsePool) {
                MemoryPool::MemoryPoolV2::deallocate(slot.pointer, slot.size);
            } else {
                delete[] static_cast<char*>(slot.pointer);
            }
            slot.pointer = nullptr;
        }
    }
    for (const auto& slot : slots) {
        if (slot.pointer) throw std::runtime_error("操作序列结束后仍有内存未释放");
    }
}

Sample measure(const Config& config) {
    std::vector<Workload> workloads;
    workloads.reserve(config.threads);
    Sample sample;
    sample.digest = 14695981039346656037ULL;
    for (size_t worker = 0; worker < config.threads; ++worker) {
        const size_t count = config.allocations / config.threads +
            (worker < config.allocations % config.threads ? 1 : 0);
        workloads.push_back(makeWorkload(config.workload, count, worker, config.threads));
        sample.allocations += workloads.back().allocations;
        hashValue(sample.digest, workloads.back().digest);
    }
    std::mutex mutex;
    std::condition_variable condition;
    size_t ready = 0, finished = 0;
    bool start = false, exit = false;
    Clock::time_point lastFinish;
    std::exception_ptr failure;
    auto recordFailure = [&] {
        std::lock_guard<std::mutex> lock(mutex);
        if (!failure) failure = std::current_exception();
    };
    auto execute = [&](const Workload& workload, std::vector<Slot>& slots) {
        if (config.allocator == "pool") replay<true>(workload, slots);
        else replay<false>(workload, slots);
    };
    std::vector<std::thread> workers;
    workers.reserve(config.threads);
    for (size_t worker = 0; worker < config.threads; ++worker) {
        workers.emplace_back([&, worker] {
            std::vector<Slot> slots;
            try {
                slots.resize(workloads[worker].slotCount);
                // Warm the allocator on the worker that will actually use it.
                if (config.phase == "warm") execute(workloads[worker], slots);
            } catch (...) { recordFailure(); }
            {
                std::unique_lock<std::mutex> lock(mutex);
                ++ready;
                condition.notify_all();
                condition.wait(lock, [&] { return start; });
                if (failure) {
                    ++finished;
                    condition.notify_all();
                    condition.wait(lock, [&] { return exit; });
                    return;
                }
            }
            try { execute(workloads[worker], slots); }
            catch (...) { recordFailure(); }
            const auto finish = Clock::now();
            {
                std::unique_lock<std::mutex> lock(mutex);
                lastFinish = std::max(lastFinish, finish);
                ++finished;
                condition.notify_all();
                // Keep worker teardown and bookkeeping destruction out of timing.
                condition.wait(lock, [&] { return exit; });
            }
        });
    }
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return ready == config.threads; });
    const auto begin = Clock::now();
    start = true;
    condition.notify_all();
    condition.wait(lock, [&] { return finished == config.threads; });
    if (!failure) {
        sample.milliseconds = std::chrono::duration<double, std::milli>(lastFinish - begin).count();
    }
    exit = true;
    condition.notify_all();
    lock.unlock();
    for (auto& worker : workers) worker.join();
    if (failure) std::rethrow_exception(failure);
    return sample;
}

class Handle {
public:
    HANDLE value = nullptr;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle() = default;
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

Sample isolatedSample(const std::filesystem::path& executable, const Config& config) {
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle reader, writer;
    if (!CreatePipe(&reader.value, &writer.value, &security, 0) ||
        !SetHandleInformation(reader.value, HANDLE_FLAG_INHERIT, 0)) {
        throw std::runtime_error("无法创建测量结果通信管道");
    }
    std::ostringstream arguments;
    arguments << " --sample " << config.allocator << ' ' << config.workload << ' '
              << config.phase << ' ' << config.threads << ' ' << config.allocations;
    const auto text = arguments.str();
    std::wstring command = L"\"" + executable.wstring() + L"\"" +
        std::wstring(text.begin(), text.end());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = writer.value;
    startup.hStdError = writer.value;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        throw std::runtime_error("无法启动独立测量进程");
    }
    Handle processHandle, threadHandle;
    processHandle.value = process.hProcess;
    threadHandle.value = process.hThread;
    CloseHandle(writer.value);
    writer.value = nullptr;
    const auto waitResult = WaitForSingleObject(processHandle.value, 30000);
    if (waitResult != WAIT_OBJECT_0) {
        TerminateProcess(processHandle.value, 1);
        WaitForSingleObject(processHandle.value, 5000);
        throw std::runtime_error("测量进程超时，或无法等待进程结束");
    }
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(processHandle.value, &exitCode)) {
        throw std::runtime_error("无法读取测量进程的退出状态");
    }
    std::string output;
    char buffer[1024];
    DWORD count = 0;
    while (ReadFile(reader.value, buffer, sizeof(buffer), &count, nullptr) && count) {
        output.append(buffer, count);
    }
    if (exitCode != 0) {
        std::ostringstream error;
        error << "独立测量失败：" << config.allocator << '/' << config.workload << '/'
              << config.phase << '/' << config.threads << " 个线程；退出状态=" << exitCode
              << "; " << output;
        throw std::runtime_error(error.str());
    }
    Sample result;
    std::string marker;
    std::istringstream input(output);
    if (!(input >> marker >> result.milliseconds >> result.allocations >> result.digest) ||
        marker != "RESULT" || result.allocations != config.allocations ||
        !(result.milliseconds > 0) || !std::isfinite(result.milliseconds)) {
        throw std::runtime_error("测量结果格式无效：" + output);
    }
    return result;
}

size_t parseCount(const std::string& text, size_t minimum, size_t maximum) {
    size_t consumed = 0;
    const auto value = std::stoull(text, &consumed);
    if (consumed != text.size() || value < minimum || value > maximum) {
        throw std::runtime_error("参数超出允许范围：" + text);
    }
    return static_cast<size_t>(value);
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
}

const char* workloadLabel(const std::string& value) {
    return value == "small" ? "小对象" : "混合大小";
}

const char* phaseLabel(const std::string& value) {
    return value == "first-use" ? "首次使用" : "预热后";
}

class ReportLog {
public:
    void open(const std::filesystem::path& path) {
        file.open(path, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("无法创建中文性能测试日志");
    }
    void write(const std::string& text) {
        std::cout << text << std::flush;
        if (file.is_open()) {
            file << text << std::flush;
            if (!file) throw std::runtime_error("写入中文性能测试日志失败");
        }
    }
    void error(const std::string& text) {
        std::cerr << text << std::flush;
        if (file.is_open()) file << text << std::flush;
    }
private:
    std::ofstream file;
};

std::string report(const Config& config, const std::vector<double>& pool,
                   const std::vector<double>& system) {
    const auto poolRange = std::minmax_element(pool.begin(), pool.end());
    const auto systemRange = std::minmax_element(system.begin(), system.end());
    std::ostringstream output;
    output << "【" << workloadLabel(config.workload) << "｜" << config.threads
           << " 个线程｜" << phaseLabel(config.phase) << "】\n"
           << std::fixed << std::setprecision(3)
           << "  内存池：" << median(pool) << " 毫秒（最小 " << *poolRange.first
           << "，最大 " << *poolRange.second << "）\n"
           << "  new/delete：" << median(system) << " 毫秒（最小 "
           << *systemRange.first << "，最大 " << *systemRange.second << "）\n"
           << "  耗时比（内存池 / new/delete）：" << median(pool) / median(system) << "\n\n";
    return output.str();
}
} // namespace

int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetConsoleOutputCP(CP_UTF8);
    ReportLog log;
    try {
        if (argc > 1 && std::string(argv[1]) == "--sample") {
            if (argc != 7) throw std::runtime_error("独立测量参数格式错误");
            Config config{argv[2], argv[3], argv[4],
                          parseCount(argv[5], 1, 8), parseCount(argv[6], 1, 2000000)};
            if ((config.allocator != "pool" && config.allocator != "new") ||
                (config.workload != "small" && config.workload != "mixed") ||
                (config.phase != "first-use" && config.phase != "warm")) {
                throw std::runtime_error("无法识别独立测量模式");
            }
            const auto result = measure(config);
            std::cout << std::setprecision(12) << "RESULT " << result.milliseconds << ' '
                      << result.allocations << ' ' << result.digest << '\n';
            return 0;
        }
        size_t rounds = 5, allocations = 200000;
        for (int argument = 1; argument < argc; ++argument) {
            const std::string option = argv[argument];
            if (option == "--quick") { rounds = 3; allocations = 20000; }
            else if ((option == "--rounds" || option == "--allocations") && argument + 1 < argc) {
                const auto value = parseCount(argv[++argument], option == "--rounds" ? 3 : 10000,
                                             option == "--rounds" ? 15 : 2000000);
                if (option == "--rounds") rounds = value;
                else allocations = value;
            } else {
                throw std::runtime_error("用法：PerformanceTest [--quick] [--rounds 3..15] [--allocations 10000..2000000]");
            }
        }
        std::wstring module(32768, L'\0');
        const auto length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
        if (!length || length >= module.size()) throw std::runtime_error("无法确定性能测试程序的位置");
        module.resize(length);
        const std::filesystem::path executable(module);
        log.open(executable.parent_path() / L"性能测试.log");
        std::ostringstream introduction;
        introduction << "内存分配器性能测试\n"
                     << "测试轮数：" << rounds << " 轮；每次测量共 " << allocations
                     << " 次分配和 " << allocations << " 次释放（所有线程合计）。\n"
                     << "测试负载：小对象 8～256 字节；混合大小 8～4096 字节，小、中、大三组的选择概率为 60%／30%／10%。\n"
                     << "总存活对象上限：" << TOTAL_LIVE_SLOTS << " 个，在线程之间分摊。\n"
                     << "每次测量使用新的独立进程，每轮交替两种分配器的执行顺序。\n"
                     << "首次使用：不进行显式预热。预热后：每个工作线程在计时前完整执行一次相同负载。\n"
                     << "计时不包含：操作序列生成、线程创建、预热和线程退出。\n"
                     << "计时包含：内存分配与释放、槽位管理，以及相同的首尾字节读写校验。\n"
                     << "以下耗时均为中位数，同时列出最小值和最大值。\n"
                     << "耗时比小于 1 表示内存池更快，大于 1 表示 new/delete 更快。\n\n";
        log.write(introduction.str());
        size_t pairedSamples = 0;
        for (const std::string workload : {"small", "mixed"}) {
            for (const size_t threads : {1, 2, 4, 8}) {
                for (const std::string phase : {"first-use", "warm"}) {
                    Config config{"pool", workload, phase, threads, allocations};
                    std::vector<double> poolTimes, systemTimes;
                    for (size_t round = 0; round < rounds; ++round) {
                        Sample pool, system;
                        auto runPool = [&] { config.allocator = "pool"; pool = isolatedSample(executable, config); };
                        auto runSystem = [&] { config.allocator = "new"; system = isolatedSample(executable, config); };
                        if (round % 2 == 0) { runPool(); runSystem(); }
                        else { runSystem(); runPool(); }
                        if (pool.digest != system.digest || pool.allocations != system.allocations) {
                            throw std::runtime_error("两种分配器执行的操作序列或分配次数不一致");
                        }
                        poolTimes.push_back(pool.milliseconds);
                        systemTimes.push_back(system.milliseconds);
                        ++pairedSamples;
                    }
                    log.write(report(config, poolTimes, systemTimes));
                }
            }
        }
        log.write("测试完成：已核对 " + std::to_string(pairedSamples) +
                  " 对操作序列，所有数据读写校验均通过。\n");
        return 0;
    } catch (const std::exception& error) {
        log.error(std::string("性能测试失败：") + error.what() + "\n");
        return 1;
    }
}
