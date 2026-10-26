#include "Hardware/Core/Emulator/EmulationEngine.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"

#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

namespace Core {

EmulationEngine::EmulationEngine(Emulator& emulator) : emulator(emulator) {}

EmulationEngine::~EmulationEngine() {
    Stop();
}

void EmulationEngine::Start() {
    if (running) {
        return;
    }
    running = true;
    paused = true;
    emulatorThread = std::thread(&EmulationEngine::ThreadLoop, this);
}

void EmulationEngine::Stop() {
    emulator.scriptEngine.Stop();
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        running = false;
        paused = false;
        pauseCV.notify_all();
    }
#ifndef TARGET_WASM
    emulator.sid.StopRecording();
#endif
    if (emulator.cartridge.loaded && !emulator.cartridge.sdCardPath.empty()) {
        Core::CartridgeLoader::SaveSDToZip(emulator.cartridge);
    }
    if (emulatorThread.joinable()) {
        emulatorThread.join();
    }
}

void EmulationEngine::Pause() {
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (paused.exchange(true)) {
            return;
        }
        pauseCV.notify_all();
    }
    emulator.sid.SetEmulationPaused(true);
}

void EmulationEngine::Resume() {
    totalCyclesAtLastResume = emulator.totalCycles;
    emulator.sid.SetEmulationPaused(false);
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        paused = false;
        pauseCV.notify_all();
    }
}

void EmulationEngine::WaitUntilSafeToMutate() {
    if (!running || std::this_thread::get_id() == emulatorThread.get_id()) {
        paused = true;
        isActuallyPaused = true;
        return;
    }
    if (!paused) {
        Pause();
    }
    while (!isActuallyPaused.load(std::memory_order_acquire)) {
        if (!running) {
            return;
        }
        std::this_thread::yield();
    }
}

void EmulationEngine::ThreadLoop() {
    using namespace std::chrono;
    auto nextSliceTime = high_resolution_clock::now();
    uint64_t cyclesThisSecond = 0;
    uint64_t cpuCyclesThisSecond = 0;
    uint64_t gpuCyclesThisSecond = 0;
    uint64_t instructionsThisSecond = 0;
    uint64_t slicesThisSecond = 0;
    uint64_t emulationThisSecond = 0;
    uint64_t sleepThisSecond = 0;
    uint64_t lateThisSecond = 0;
    auto lastSecondTime = high_resolution_clock::now();

    double instructionAccumulator = 0.0;
    auto lastWatchCheck = high_resolution_clock::now();

    while (running.load(std::memory_order_relaxed)) {
        if (paused.load(std::memory_order_relaxed)) {
            isActuallyPaused.store(true, std::memory_order_release);
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
            isActuallyPaused.store(false, std::memory_order_release);
            nextSliceTime = high_resolution_clock::now();
            lastSecondTime = nextSliceTime;
        }
        if (!running.load(std::memory_order_relaxed)) {
            break;
        }

        if (emulator.scriptEngine.HasPendingScript()) {
            emulator.scriptEngine.ExecutePendingScript();
            continue;
        }

        int currentTarget = targetIPS.load();
        if (currentTarget <= 0) {
            currentTarget = 1;
        }

        int sliceDurationMs = 10;
        double targetPerSlice = (double)currentTarget / (1000.0 / sliceDurationMs);

        instructionAccumulator += targetPerSlice;
        int instructionsPerSlice = (int)instructionAccumulator;
        instructionAccumulator -= instructionsPerSlice;

        isActuallyPaused.store(false, std::memory_order_release);
        const auto sliceStart = high_resolution_clock::now();
        const SliceResult slice = EmulateSlice(instructionsPerSlice);
        const auto sliceEnd = high_resolution_clock::now();
        emulationThisSecond += static_cast<uint64_t>(duration_cast<nanoseconds>(sliceEnd - sliceStart).count());
        cyclesThisSecond += slice.cycles;
        cpuCyclesThisSecond += slice.cpuCycles;
        gpuCyclesThisSecond += slice.gpuCycles;
        instructionsThisSecond += slice.instructions;
        slicesThisSecond++;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            telemetry.CommitSecond(cyclesThisSecond, cpuCyclesThisSecond, gpuCyclesThisSecond,
                                   instructionsThisSecond, slicesThisSecond, emulationThisSecond,
                                   sleepThisSecond, lateThisSecond);
            cyclesThisSecond = 0;
            cpuCyclesThisSecond = 0;
            gpuCyclesThisSecond = 0;
            instructionsThisSecond = 0;
            slicesThisSecond = 0;
            emulationThisSecond = 0;
            sleepThisSecond = 0;
            lateThisSecond = 0;
            lastSecondTime = now;
        }

        nextSliceTime += milliseconds(sliceDurationMs);

        auto sleepCheckTime = high_resolution_clock::now();
        if (sleepCheckTime < nextSliceTime) {
            sleepThisSecond += static_cast<uint64_t>(duration_cast<nanoseconds>(nextSliceTime - sleepCheckTime).count());
            std::this_thread::sleep_until(nextSliceTime);
        } else {
            lateThisSecond += static_cast<uint64_t>(duration_cast<nanoseconds>(sleepCheckTime - nextSliceTime).count());
            nextSliceTime = sleepCheckTime;
            std::this_thread::yield();
        }

        CheckAutoReload(lastWatchCheck);
    }
}

EmulationEngine::SliceResult EmulationEngine::EmulateSlice(int instructionsPerSlice) {
    using namespace std::chrono;
    if (instructionsPerSlice <= 0) {
        return {};
    }

    static constexpr int MAX_BATCH_SIZE = 500000;
    static constexpr int NO_STOP = std::numeric_limits<int>::min();
    int remaining = instructionsPerSlice;
    uint64_t localCpuCycleDelta = 0;
    uint64_t localGpuCycleDelta = 0;
    uint64_t localInstructionDelta = 0;
    bool totalsCommitted = false;

    emulator.savestateManager.OnSliceStep();

    emulator.EnsureWatchpointWriteHook();
    bool hasBreakpoints = emulator.breakpointManager.HasAnyBreakpointsFast();
    bool hasComplex = emulator.breakpointManager.HasComplexBreakpoints();
    bool hooks = emulator.bus.HasActiveHooks() || hasBreakpoints || emulator.logicAnalyzerEnabled.load(std::memory_order_relaxed);

    while (remaining > 0) {
        int currentBatchSize = std::min(remaining, MAX_BATCH_SIZE);
        int batchInstructions = 0;
        uint64_t batchCpuCycles = 0;
        uint64_t batchGpuCycles = 0;
        int stopCode = NO_STOP;

        const auto batchStart = high_resolution_clock::now();
        int stepsInBatch = ProcessBatch(
            currentBatchSize, hooks, hasBreakpoints, hasComplex, batchInstructions, batchCpuCycles, batchGpuCycles, stopCode);
        const auto batchEnd = high_resolution_clock::now();
        const auto batchNs = static_cast<uint64_t>(duration_cast<nanoseconds>(batchEnd - batchStart).count());
        const uint64_t batchCycles = batchCpuCycles + batchGpuCycles;
        telemetry.RecordBatch(batchNs, batchCycles);
        localCpuCycleDelta += batchCpuCycles;
        localGpuCycleDelta += batchGpuCycles;
        localInstructionDelta += static_cast<uint64_t>(batchInstructions);

        remaining -= stepsInBatch;

        if (stopCode != NO_STOP) {
            emulator.totalCycles.fetch_add(localCpuCycleDelta + localGpuCycleDelta, std::memory_order_relaxed);
            emulator.totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
            totalsCommitted = true;
            HandleStop(stopCode);
            break;
        }

        if (stepsInBatch < currentBatchSize) {
            break;
        }
    }

    if (!totalsCommitted && (localCpuCycleDelta + localGpuCycleDelta) > 0) {
        emulator.totalCycles.fetch_add(localCpuCycleDelta + localGpuCycleDelta, std::memory_order_relaxed);
    }
    if (!totalsCommitted && localInstructionDelta > 0) {
        emulator.totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
    }

    return {
        .cycles = localCpuCycleDelta + localGpuCycleDelta,
        .cpuCycles = localCpuCycleDelta,
        .gpuCycles = localGpuCycleDelta,
        .instructions = localInstructionDelta,
    };
}

int EmulationEngine::ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                                  uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode) {
    return emulator.ProcessBatch(count, hooks, hasBreakpoints, hasComplex, batchInstructions, batchCpuCycles,
                                 batchGpuCycles, stopCode);
}

void EmulationEngine::HandleStop(int code) {
    Pause();
#ifndef TARGET_WASM
    if (emulator.totalCycles > totalCyclesAtLastResume) {
        emulator.sid.StopRecording();
    }
#endif
    halted = true;
    if (code != 0) {
        std::cerr << "Emulator stopped with code: " << code << '\n';
    }
    emulator.sid.SetEmulationPaused(true);
}

void EmulationEngine::CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck) {
    fileWatcher.CheckAutoReload(emulator, lastWatchCheck);
}

}  // namespace Core
