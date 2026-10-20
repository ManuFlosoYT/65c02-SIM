#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"

#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

namespace Core {

void Emulator::Start() {
    if (running) {
        return;
    }
    running = true;
    paused = true;
    emulatorThread = std::thread(&Emulator::ThreadLoop, this);
}

void Emulator::Stop() {
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        running = false;
        paused = false;
        pauseCV.notify_all();
    }
#ifndef TARGET_WASM
    sid.StopRecording();
#endif
    if (cartridge.loaded && !cartridge.sdCardPath.empty()) {
        Core::CartridgeLoader::SaveSDToZip(cartridge);
    }
    if (emulatorThread.joinable()) {
        emulatorThread.join();
    }
}

void Emulator::Pause() {
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (paused.exchange(true)) {
            return;
        }
        pauseCV.notify_all();
    }
    sid.SetEmulationPaused(true);
}

void Emulator::Resume() {
    totalCyclesAtLastResume = totalCycles;
    sid.SetEmulationPaused(false);
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        paused = false;
        pauseCV.notify_all();
    }
}

void Emulator::WaitUntilSafeToMutate() {
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

void Emulator::ThreadLoop() {
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

        if (scriptEngine.HasPendingScript()) {
            scriptEngine.ExecutePendingScript();
            continue; // Script has taken over until it paused or finished
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

Emulator::SliceResult Emulator::EmulateSlice(int instructionsPerSlice) {
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

    // Save rewind state every N slices when rewind snapshots are enabled.
    if (rewindSnapshotsEnabled) {
        rewindSliceCounter++;
        if (rewindSliceCounter >= rewindInterval) {
            SaveStateToBuffer();
            rewindSliceCounter = 0;
        }
    }

    EnsureWatchpointWriteHook();
    bool hasBreakpoints = breakpointManager.HasAnyBreakpointsFast();
    bool hasComplex = breakpointManager.HasComplexBreakpoints();
    bool hooks = bus.HasActiveHooks() || hasBreakpoints || logicAnalyzerEnabled.load(std::memory_order_relaxed);

    while (remaining > 0) {
        int currentBatchSize = std::min(remaining, MAX_BATCH_SIZE);
        int batchInstructions = 0;
        uint64_t batchCpuCycles = 0;
        uint64_t batchGpuCycles = 0;
        int stopCode = NO_STOP;
        int stepsInBatch = 0;

        stepsInBatch = this->ProcessBatch(
            currentBatchSize, hooks, hasBreakpoints, hasComplex, batchInstructions, batchCpuCycles, batchGpuCycles, stopCode);
        localCpuCycleDelta += batchCpuCycles;
        localGpuCycleDelta += batchGpuCycles;
        localInstructionDelta += static_cast<uint64_t>(batchInstructions);

        remaining -= stepsInBatch;

        if (stopCode != NO_STOP) {
            totalCycles.fetch_add(localCpuCycleDelta + localGpuCycleDelta, std::memory_order_relaxed);
            totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
            totalsCommitted = true;
            handleStop(stopCode);
            break;
        }

        if (stepsInBatch < currentBatchSize) {
            break;
        }
    }

    if (!totalsCommitted && (localCpuCycleDelta + localGpuCycleDelta) > 0) {
        totalCycles.fetch_add(localCpuCycleDelta + localGpuCycleDelta, std::memory_order_relaxed);
    }
    if (!totalsCommitted && localInstructionDelta > 0) {
        totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
    }

    return {
        .cycles = localCpuCycleDelta + localGpuCycleDelta,
        .cpuCycles = localCpuCycleDelta,
        .gpuCycles = localGpuCycleDelta,
        .instructions = localInstructionDelta,
    };
}

int Emulator::ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                           uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode) {
    stopCode = std::numeric_limits<int>::min();
    int ips = targetIPS.load(std::memory_order_relaxed);
    if (ips <= 0) {
        ips = 1000000;
    }
    if (sidEnabled) {
        sid.SetCpuClockRate(static_cast<double>(ips));
    }

    if (cpu.cycleAccurate) {
        if (!hooks && !hasBreakpoints) [[likely]] {
            uint64_t localCpuCycles = 0;
            uint64_t localGpuCycles = 0;
            int localInstructions = 0;
            int i = 0;
            while (i < count) {
                if (gpuEnabled && !gpu.IsInBlankingInterval()) {
                    const uint32_t waitCycles = gpu.GetDrawingWaitCycles();
                    if (waitCycles > 0 && (!sidEnabled || sid.IsDormant()) && via.IsIdle() && acia.IsIdle() &&
                        baudDelay == 0 && !pendingInterruptAny.load(std::memory_order_relaxed) &&
                        !hasInput.load(std::memory_order_relaxed)) [[likely]] {
                        const auto rem = static_cast<uint32_t>(count - i);
                        const uint32_t skip = (rem < waitCycles) ? rem : waitCycles;
                        gpu.SkipDrawingCycles(skip);
                        via.Clock(skip);
                        if (sidEnabled) {
                            sid.Clock(skip);
                        }
                        localGpuCycles += skip;
                        i += static_cast<int>(skip);
                        continue;
                    }
                }

                const bool isNew = (cpu.remainingCycles == 0);
                bool cpuStepped = false;
                int res = 0;
                if (gpuEnabled) {
                    gpu.Clock();
                    if (gpu.IsInBlankingInterval()) {
                        res = cpu.StepCycleAccurate<false>(bus);
                        cpuStepped = true;
                    }
                } else {
                    res = cpu.StepCycleAccurate<false>(bus);
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

                if (baudDelay > 0) [[unlikely]] {
                    baudDelay--;
                }
                via.Clock();
                acia.Clock(ips);
                if (sidEnabled) {
                    sid.Clock();
                }
                if (via.isIRQAsserted() || acia.HasIRQ()) [[unlikely]] {
                    pendingInterruptAny.store(true, std::memory_order_relaxed);
                }
                if (pendingInterruptAny.load(std::memory_order_relaxed)) [[unlikely]] {
                    HandleInterrupts<false>();
                }
                if (hasInput.load(std::memory_order_relaxed)) [[unlikely]] {
                    HandleSerialInput<false>();
                }

                ++i;
                if (res != 0) [[unlikely]] {
                    batchInstructions += localInstructions;
                    batchCpuCycles += localCpuCycles;
                    batchGpuCycles += localGpuCycles;
                    lastStepCpuCycles = cpuStepped ? 1 : 0;
                    lastStepGpuCycles = (gpuEnabled && !cpuStepped) ? 1 : 0;
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
            bool isNew = (cpu.remainingCycles == 0);
            int res = hooks ? StepInternal<true, true>(ips) : StepInternal<false, true>(ips);

            if (isNew && lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += lastStepCpuCycles;
            batchGpuCycles += lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }

            if (hasBreakpoints) {
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        stopCode = 0;
                        return i + 1;
                    }
                }
            }
        }
    } else {
        if (!hooks && !hasBreakpoints) [[likely]] {
            for (int i = 0; i < count; ++i) {
                int res = StepInternal<false, false>(ips);
                if (lastStepCpuCycles > 0) {
                    batchInstructions++;
                }
                batchCpuCycles += lastStepCpuCycles;
                batchGpuCycles += lastStepGpuCycles;
                if (res != 0) [[unlikely]] {
                    stopCode = res;
                    return i + 1;
                }
            }
            return count;
        }
        for (int i = 0; i < count; ++i) {
            int res = hooks ? StepInternal<true, false>(ips) : StepInternal<false, false>(ips);

            if (lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += lastStepCpuCycles;
            batchGpuCycles += lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }

            if (hasBreakpoints) {
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        stopCode = 0;
                        return i + 1;
                    }
                }
            }
        }
    }
    return count;
}

void Emulator::handleStop(int code) {
    Pause();
#ifndef TARGET_WASM
    if (totalCycles > totalCyclesAtLastResume) {
        sid.StopRecording();
    }
#endif
    halted = true;
    if (code != 0) {
        std::cerr << "Emulator stopped with code: " << code << '\n';
    }
    sid.SetEmulationPaused(true);
}


void Emulator::CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck) {
#ifndef TARGET_WASM
    using namespace std::chrono;
    if (autoReloadRequested.load() && !currentBinPath.empty()) {
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - lastWatchCheck).count() >= 500) {
            lastWatchCheck = now;
            try {
                if (std::filesystem::exists(currentBinPath)) {
                    auto latestTime = std::filesystem::last_write_time(currentBinPath);
                    if (latestTime > lastBinModificationTime) {
                        std::string errorMsg;
                        std::cerr << "Auto-reloading: " << currentBinPath << '\n';
                        if (!Init(currentBinPath, errorMsg)) {
                            std::cerr << "Auto-reload failed: " << errorMsg << '\n';
                        }
                    }
                }
            } catch (...) {
                // Ignore filesystem errors
                (void)0;
            }
        }
    }
#else
    (void)lastWatchCheck;
#endif
}

}  // namespace Core
