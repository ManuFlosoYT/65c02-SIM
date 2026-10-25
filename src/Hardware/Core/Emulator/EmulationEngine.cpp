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
    auto lastSecondTime = high_resolution_clock::now();

    double instructionAccumulator = 0.0;
    auto lastWatchCheck = high_resolution_clock::now();

    while (running.load(std::memory_order_relaxed)) {
        if (paused.load(std::memory_order_relaxed)) {
            isActuallyPaused.store(true, std::memory_order_release);
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
            isActuallyPaused.store(false, std::memory_order_release);
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
        emulationNanoseconds.fetch_add(
            static_cast<uint64_t>(duration_cast<nanoseconds>(sliceEnd - sliceStart).count()),
            std::memory_order_relaxed);
        cyclesThisSecond += slice.cycles;
        cpuCyclesThisSecond += slice.cpuCycles;
        gpuCyclesThisSecond += slice.gpuCycles;
        instructionsThisSecond += slice.instructions;
        slicesThisSecond++;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            actualCyclesPerSecond.store(cyclesThisSecond, std::memory_order_relaxed);
            actualCpuCyclesPerSecond.store(cpuCyclesThisSecond, std::memory_order_relaxed);
            actualGpuCyclesPerSecond.store(gpuCyclesThisSecond, std::memory_order_relaxed);
            actualInstructionsPerSecond.store(instructionsThisSecond, std::memory_order_relaxed);
            slicesPerSecond.store(slicesThisSecond, std::memory_order_relaxed);
            cyclesThisSecond = 0;
            cpuCyclesThisSecond = 0;
            gpuCyclesThisSecond = 0;
            instructionsThisSecond = 0;
            slicesThisSecond = 0;
            lastSecondTime = now;
        }

        nextSliceTime += milliseconds(sliceDurationMs);

        auto sleepCheckTime = high_resolution_clock::now();
        if (sleepCheckTime < nextSliceTime) {
            sleepNanoseconds.fetch_add(
                static_cast<uint64_t>(duration_cast<nanoseconds>(nextSliceTime - sleepCheckTime).count()),
                std::memory_order_relaxed);
            std::this_thread::sleep_until(nextSliceTime);
        } else {
            lateNanoseconds.fetch_add(
                static_cast<uint64_t>(duration_cast<nanoseconds>(sleepCheckTime - nextSliceTime).count()),
                std::memory_order_relaxed);
            nextSliceTime = sleepCheckTime;
            std::this_thread::yield();
        }

        CheckAutoReload(lastWatchCheck);
    }
}

EmulationEngine::SliceResult EmulationEngine::EmulateSlice(int instructionsPerSlice) {
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

        int stepsInBatch = ProcessBatch(
            currentBatchSize, hooks, hasBreakpoints, hasComplex, batchInstructions, batchCpuCycles, batchGpuCycles, stopCode);
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
    stopCode = std::numeric_limits<int>::min();
    int ips = targetIPS.load(std::memory_order_relaxed);
    if (ips <= 0) {
        ips = 1000000;
    }
    if (emulator.sidEnabled) {
        emulator.sid.SetCpuClockRate(static_cast<double>(ips));
    }

    if (emulator.cpu.cycleAccurate) {
        if (!hooks && !hasBreakpoints) [[likely]] {
            uint64_t localCpuCycles = 0;
            uint64_t localGpuCycles = 0;
            int localInstructions = 0;
            int i = 0;
            while (i < count) {
                if (emulator.gpuEnabled && !emulator.gpu.IsInBlankingInterval()) {
                    const uint32_t waitCycles = emulator.gpu.GetDrawingWaitCycles();
                    if (waitCycles > 0 && (!emulator.sidEnabled || emulator.sid.IsDormant()) && emulator.via.IsIdle() &&
                        emulator.acia.IsIdle() && emulator.baudDelay == 0 &&
                        !emulator.pendingInterruptAny.load(std::memory_order_relaxed) &&
                        !emulator.hasInput.load(std::memory_order_relaxed)) [[likely]] {
                        const auto rem = static_cast<uint32_t>(count - i);
                        const uint32_t skip = (rem < waitCycles) ? rem : waitCycles;
                        emulator.gpu.SkipDrawingCycles(skip);
                        emulator.via.Clock(skip);
                        if (emulator.sidEnabled) {
                            emulator.sid.Clock(skip);
                        }
                        localGpuCycles += skip;
                        i += static_cast<int>(skip);
                        continue;
                    }
                }

                const bool isNew = (emulator.cpu.remainingCycles == 0);
                bool cpuStepped = false;
                int res = 0;
                if (emulator.gpuEnabled) {
                    emulator.gpu.Clock();
                    if (emulator.gpu.IsInBlankingInterval()) {
                        res = emulator.cpu.StepCycleAccurate<false>(emulator.bus);
                        cpuStepped = true;
                    }
                } else {
                    res = emulator.cpu.StepCycleAccurate<false>(emulator.bus);
                    cpuStepped = true;
                }

                if (cpuStepped) {
                    localCpuCycles++;
                    if (isNew) {
                        localInstructions++;
                    }
                } else {
                    localGpuCycles++;
                }

                if (emulator.baudDelay > 0) [[unlikely]] {
                    emulator.baudDelay--;
                }
                emulator.via.Clock();
                emulator.acia.Clock(ips);
                if (emulator.sidEnabled) {
                    emulator.sid.Clock();
                }
                if (emulator.via.isIRQAsserted() || emulator.acia.HasIRQ()) [[unlikely]] {
                    emulator.pendingInterruptAny.store(true, std::memory_order_relaxed);
                }
                if (emulator.pendingInterruptAny.load(std::memory_order_relaxed)) [[unlikely]] {
                    emulator.HandleInterrupts<false>();
                }
                if (emulator.hasInput.load(std::memory_order_relaxed)) [[unlikely]] {
                    emulator.HandleSerialInput<false>();
                }

                ++i;
                if (res != 0) [[unlikely]] {
                    batchInstructions += localInstructions;
                    batchCpuCycles += localCpuCycles;
                    batchGpuCycles += localGpuCycles;
                    emulator.lastStepCpuCycles = cpuStepped ? 1 : 0;
                    emulator.lastStepGpuCycles = (emulator.gpuEnabled && !cpuStepped) ? 1 : 0;
                    stopCode = res;
                    return i;
                }
            }
            batchInstructions += localInstructions;
            batchCpuCycles += localCpuCycles;
            batchGpuCycles += localGpuCycles;
            return count;
        }
        for (int i = 0; i < count; ++i) {
            bool isNew = (emulator.cpu.remainingCycles == 0);
            if (hasBreakpoints && isNew) {
                if (!emulator.cpu.isInit) {
                    emulator.cpu.EnsureInit(emulator.bus);
                }
                if (hasComplex || emulator.breakpointManager.IsPCBreakpoint(emulator.cpu.PC)) [[unlikely]] {
                    if (emulator.breakpointManager.Evaluate(emulator.cpu, emulator.bus) > 0) {
                        stopCode = 0;
                        return i;
                    }
                }
            }
            int res = hooks ? emulator.StepInternal<true, true>(ips) : emulator.StepInternal<false, true>(ips);

            if (isNew && emulator.lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += emulator.lastStepCpuCycles;
            batchGpuCycles += emulator.lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }
        }
    } else {
        if (!hooks && !hasBreakpoints) [[likely]] {
            for (int i = 0; i < count; ++i) {
                int res = emulator.StepInternal<false, false>(ips);
                if (emulator.lastStepCpuCycles > 0) {
                    batchInstructions++;
                }
                batchCpuCycles += emulator.lastStepCpuCycles;
                batchGpuCycles += emulator.lastStepGpuCycles;
                if (res != 0) [[unlikely]] {
                    stopCode = res;
                    return i + 1;
                }
            }
            return count;
        }
        for (int i = 0; i < count; ++i) {
            if (hasBreakpoints) {
                if (!emulator.cpu.isInit) {
                    emulator.cpu.EnsureInit(emulator.bus);
                }
                if (hasComplex || emulator.breakpointManager.IsPCBreakpoint(emulator.cpu.PC)) [[unlikely]] {
                    if (emulator.breakpointManager.Evaluate(emulator.cpu, emulator.bus) > 0) {
                        stopCode = 0;
                        return i;
                    }
                }
            }
            int res = hooks ? emulator.StepInternal<true, false>(ips) : emulator.StepInternal<false, false>(ips);

            if (emulator.lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += emulator.lastStepCpuCycles;
            batchGpuCycles += emulator.lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }
        }
    }
    return count;
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
#ifndef TARGET_WASM
    using namespace std::chrono;
    if (autoReloadRequested.load() && !emulator.currentBinPath.empty()) {
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - lastWatchCheck).count() >= 500) {
            lastWatchCheck = now;
            try {
                if (std::filesystem::exists(emulator.currentBinPath)) {
                    auto latestTime = std::filesystem::last_write_time(emulator.currentBinPath);
                    if (latestTime > lastBinModificationTime) {
                        std::string errorMsg;
                        std::cerr << "Auto-reloading: " << emulator.currentBinPath << '\n';
                        if (!emulator.Init(emulator.currentBinPath, errorMsg)) {
                            std::cerr << "Auto-reload failed: " << errorMsg << '\n';
                        }
                    }
                }
            } catch (...) {
                (void)0;
            }
        }
    }
#else
    (void)lastWatchCheck;
#endif
}

EmulationEngine::RuntimeStats EmulationEngine::GetRuntimeStats() const {
    return {
        .cyclesPerSecond = actualCyclesPerSecond.load(std::memory_order_relaxed),
        .cpuCyclesPerSecond = actualCpuCyclesPerSecond.load(std::memory_order_relaxed),
        .gpuCyclesPerSecond = actualGpuCyclesPerSecond.load(std::memory_order_relaxed),
        .instructionsPerSecond = actualInstructionsPerSecond.load(std::memory_order_relaxed),
        .slicesPerSecond = slicesPerSecond.load(std::memory_order_relaxed),
        .emulationNanoseconds = emulationNanoseconds.load(std::memory_order_relaxed),
        .sleepNanoseconds = sleepNanoseconds.load(std::memory_order_relaxed),
        .lateNanoseconds = lateNanoseconds.load(std::memory_order_relaxed),
    };
}

}  // namespace Core
