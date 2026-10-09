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
    running = false;
#ifndef TARGET_WASM
    sid.StopRecording();
#endif
    if (cartridge.loaded && !cartridge.sdCardPath.empty()) {
        Core::CartridgeLoader::SaveSDToZip(cartridge);
    }
    Resume();
    if (emulatorThread.joinable()) {
        emulatorThread.join();
    }
}

void Emulator::Pause() {
    if (paused.exchange(true)) {
        return;
    }
    sid.SetEmulationPaused(true);
    pauseCV.notify_all();
}

void Emulator::Resume() {
    totalCyclesAtLastResume = totalCycles;
    sid.SetEmulationPaused(false);
    paused = false;
    pauseCV.notify_all();
}

void Emulator::WaitUntilSafeToMutate() {
    if (!running) {
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

        const auto sliceStart = high_resolution_clock::now();
        const SliceResult slice = EmulateSlice(instructionsPerSlice);
        const auto sliceEnd = high_resolution_clock::now();
        emulationNanoseconds.fetch_add(
            static_cast<uint64_t>(duration_cast<nanoseconds>(sliceEnd - sliceStart).count()),
            std::memory_order_relaxed);
        cyclesThisSecond += slice.cycles;
        instructionsThisSecond += slice.instructions;
        slicesThisSecond++;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            actualCyclesPerSecond.store(cyclesThisSecond, std::memory_order_relaxed);
            actualInstructionsPerSecond.store(instructionsThisSecond, std::memory_order_relaxed);
            actualIPS.store(static_cast<int>(instructionsThisSecond), std::memory_order_relaxed);
            slicesPerSecond.store(slicesThisSecond, std::memory_order_relaxed);
            cyclesThisSecond = 0;
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
    uint64_t localCycleDelta = 0;
    uint64_t localInstructionDelta = 0;
    bool totalsCommitted = false;

    // Save rewind state roughly every 1s only when rewind snapshots are enabled.
    if (rewindSnapshotsEnabled) {
        rewindSliceCounter++;
        if (rewindSliceCounter >= (1000 / 10)) {
            SaveStateToBuffer();
            rewindSliceCounter = 0;
        }
    }

    EnsureWatchpointWriteHook();
    bool hasBreakpoints = breakpointManager.HasAnyBreakpointsFast();
    bool hasComplex = breakpointManager.HasComplexBreakpoints();
    bool hooks = bus.HasActiveHooks() || hasBreakpoints;

    while (remaining > 0) {
        int currentBatchSize = std::min(remaining, MAX_BATCH_SIZE);
        int batchInstructions = 0;
        uint64_t batchCycles = 0;
        int stopCode = NO_STOP;
        int stepsInBatch = 0;

        stepsInBatch = this->ProcessBatch(
            currentBatchSize, hooks, hasBreakpoints, hasComplex, batchInstructions, batchCycles, stopCode);
        localCycleDelta += batchCycles;
        localInstructionDelta += static_cast<uint64_t>(batchInstructions);

        remaining -= stepsInBatch;

        if (stopCode != NO_STOP) {
            totalCycles.fetch_add(localCycleDelta, std::memory_order_relaxed);
            totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
            totalsCommitted = true;
            handleStop(stopCode);
            break;
        }

        if (stepsInBatch < currentBatchSize) {
            break;
        }
    }

    if (!totalsCommitted && localCycleDelta > 0) {
        totalCycles.fetch_add(localCycleDelta, std::memory_order_relaxed);
    }
    if (!totalsCommitted && localInstructionDelta > 0) {
        totalInstructions.fetch_add(localInstructionDelta, std::memory_order_relaxed);
    }

    return {.cycles = localCycleDelta, .instructions = localInstructionDelta};
}

int Emulator::ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                           uint64_t& batchCycles, int& stopCode) {
    stopCode = std::numeric_limits<int>::min();
    if (cpu.cycleAccurate) {
        for (int i = 0; i < count; ++i) {
            bool isNew = (cpu.remainingCycles == 0);
            int res = 0;

            if (hooks) [[unlikely]] {
                res = Step<true>();
            } else {
                res = Step<false>();
            }

            if (isNew) {
                batchInstructions++;
            }
            batchCycles += lastStepCycles;

            if (res != 0) {
                stopCode = res;
                return i + 1;
            }

            if (hasBreakpoints) [[unlikely]] {
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        stopCode = 0;
                        return i + 1;
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < count; ++i) {
            int res = 0;

            if (hooks) [[unlikely]] {
                res = Step<true>();
            } else {
                res = Step<false>();
            }

            batchInstructions++;
            batchCycles += lastStepCycles;

            if (res != 0) {
                stopCode = res;
                return i + 1;
            }

            if (hasBreakpoints) [[unlikely]] {
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
