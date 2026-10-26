#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>

namespace Core {

class Emulator;

class EmulationEngine {
   public:
    struct RuntimeStats {
        uint64_t cyclesPerSecond = 0;
        uint64_t cpuCyclesPerSecond = 0;
        uint64_t gpuCyclesPerSecond = 0;
        uint64_t instructionsPerSecond = 0;
        uint64_t slicesPerSecond = 0;
        uint64_t emulationNanoseconds = 0;
        uint64_t sleepNanoseconds = 0;
        uint64_t lateNanoseconds = 0;
        uint64_t avgBatchDurationNs = 0;
        uint64_t lastBatchDurationNs = 0;
        uint64_t avgBatchCycles = 0;
        uint64_t lastBatchCycles = 0;
        uint64_t batchesPerSecond = 0;
    };

    struct SliceResult {
        uint64_t cycles = 0;
        uint64_t cpuCycles = 0;
        uint64_t gpuCycles = 0;
        uint64_t instructions = 0;
    };

    explicit EmulationEngine(Emulator& emulator);
    ~EmulationEngine();

    EmulationEngine(const EmulationEngine&) = delete;
    EmulationEngine& operator=(const EmulationEngine&) = delete;
    EmulationEngine(EmulationEngine&&) = delete;
    EmulationEngine& operator=(EmulationEngine&&) = delete;

    void Start();
    void Stop();
    void Pause();
    void Resume();
    void WaitUntilSafeToMutate();

    [[nodiscard]] bool IsRunning() const { return running.load(std::memory_order_relaxed); }
    [[nodiscard]] bool IsPaused() const { return paused.load(std::memory_order_relaxed); }
    [[nodiscard]] bool IsHalted() const { return halted.load(std::memory_order_relaxed); }
    void SetHalted(bool h) { halted.store(h, std::memory_order_relaxed); }

    void SetTargetIPS(int ips) { targetIPS = ips; }
    [[nodiscard]] int GetTargetIPS() const { return targetIPS.load(std::memory_order_relaxed); }

    [[nodiscard]] RuntimeStats GetRuntimeStats() const;

    void SetAutoReload(bool enabled) { autoReloadRequested = enabled; }
    [[nodiscard]] bool IsAutoReloadEnabled() const { return autoReloadRequested.load(std::memory_order_relaxed); }
    void SetLastBinModificationTime(std::filesystem::file_time_type t) { lastBinModificationTime = t; }

    SliceResult EmulateSlice(int instructionsPerSlice);
    int ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                     uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode);
    void HandleStop(int code);
    void CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck);

    [[nodiscard]] std::thread::id GetThreadId() const { return emulatorThread.get_id(); }
    [[nodiscard]] bool IsActuallyPaused() const { return isActuallyPaused.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t GetTotalCyclesAtLastResume() const { return totalCyclesAtLastResume; }

   private:
    friend class Emulator;

    void ThreadLoop();

    Emulator& emulator;

    std::atomic<bool> running{false};
    std::atomic<bool> paused{true};
    std::atomic<bool> halted{false};
    std::atomic<int> targetIPS{1000000};

    std::atomic<uint64_t> actualCyclesPerSecond{0};
    std::atomic<uint64_t> actualCpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualGpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualInstructionsPerSecond{0};
    std::atomic<uint64_t> slicesPerSecond{0};
    std::atomic<uint64_t> emulationNanoseconds{0};
    std::atomic<uint64_t> sleepNanoseconds{0};
    std::atomic<uint64_t> lateNanoseconds{0};
    std::atomic<uint64_t> avgBatchDurationNs{0};
    std::atomic<uint64_t> lastBatchDurationNs{0};
    std::atomic<uint64_t> avgBatchCycles{0};
    std::atomic<uint64_t> lastBatchCycles{0};
    std::atomic<uint64_t> batchesPerSecond{0};

    uint64_t batchDurationThisSecond{0};
    uint64_t batchCyclesThisSecond{0};
    uint64_t batchesThisSecond{0};
    uint64_t currentLastBatchDurationNs{0};
    uint64_t currentLastBatchCycles{0};

    std::thread emulatorThread;
    std::mutex threadMutex;
    std::condition_variable pauseCV;
    std::atomic<bool> isActuallyPaused{true};

    uint64_t totalCyclesAtLastResume{0};
    std::filesystem::file_time_type lastBinModificationTime;
    std::atomic<bool> autoReloadRequested{true};
};

}  // namespace Core
