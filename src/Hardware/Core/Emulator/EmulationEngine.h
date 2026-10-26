#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>

#include "Hardware/Core/Emulator/BinaryFileWatcher.h"
#include "Hardware/Core/Emulator/SchedulerTelemetry.h"

namespace Core {

class Emulator;

class EmulationEngine {
   public:
    using RuntimeStats = Core::RuntimeStats;

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

    [[nodiscard]] RuntimeStats GetRuntimeStats() const { return telemetry.Snapshot(); }

    void SetAutoReload(bool enabled) { fileWatcher.SetEnabled(enabled); }
    [[nodiscard]] bool IsAutoReloadEnabled() const { return fileWatcher.IsEnabled(); }
    void SetLastBinModificationTime(std::filesystem::file_time_type t) { fileWatcher.SetLastBinModificationTime(t); }

    SliceResult EmulateSlice(int instructionsPerSlice);
    int ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                     uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode);
    void HandleStop(int code);
    void CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck);

    [[nodiscard]] std::thread::id GetThreadId() const { return emulatorThread.get_id(); }
    [[nodiscard]] bool IsActuallyPaused() const { return isActuallyPaused.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t GetTotalCyclesAtLastResume() const { return totalCyclesAtLastResume; }

    SchedulerTelemetry& GetTelemetry() { return telemetry; }
    const SchedulerTelemetry& GetTelemetry() const { return telemetry; }
    BinaryFileWatcher& GetFileWatcher() { return fileWatcher; }
    const BinaryFileWatcher& GetFileWatcher() const { return fileWatcher; }

   private:
    friend class Emulator;

    void ThreadLoop();

    Emulator& emulator;

    std::atomic<bool> running{false};
    std::atomic<bool> paused{true};
    std::atomic<bool> halted{false};
    std::atomic<int> targetIPS{1000000};

    SchedulerTelemetry telemetry;

    std::thread emulatorThread;
    std::mutex threadMutex;
    std::condition_variable pauseCV;
    std::atomic<bool> isActuallyPaused{true};

    uint64_t totalCyclesAtLastResume{0};
    BinaryFileWatcher fileWatcher;
};

}  // namespace Core
