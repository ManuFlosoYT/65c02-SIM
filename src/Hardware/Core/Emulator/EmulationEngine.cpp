#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"

#include <chrono>
#include <iostream>
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
    paused = true;
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
    int instructionsThisSecond = 0;
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

        int actuallyExecuted = EmulateSlice(instructionsPerSlice);
        instructionsThisSecond += actuallyExecuted;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            actualIPS = instructionsThisSecond;
            instructionsThisSecond = 0;
            lastSecondTime = now;
        }

        nextSliceTime += milliseconds(sliceDurationMs);

        auto sleepCheckTime = high_resolution_clock::now();
        if (sleepCheckTime < nextSliceTime) {
            std::this_thread::sleep_until(nextSliceTime);
        } else {
            nextSliceTime = sleepCheckTime;
            std::this_thread::yield();
        }

        CheckAutoReload(lastWatchCheck);
    }
}

int Emulator::EmulateSlice(int instructionsPerSlice) {
    if (instructionsPerSlice <= 0) {
        return 0;
    }

    const int MAX_BATCH_SIZE = 100000;
    int remaining = instructionsPerSlice;
    int totalExecuted = 0;

    // Save state roughly every 1s
    // Calculating how many slice loops (10ms each) fit in 1000ms
    static int sliceCounter = 0;
    sliceCounter++;
    if (sliceCounter >= (1000 / 10)) {
        // Lock removed
        SaveStateToBuffer();
        sliceCounter = 0;
    }

    while (remaining > 0) {
        int currentBatchSize = std::min(remaining, MAX_BATCH_SIZE);
        int batchInstructions = 0;
        int stepsInBatch = 0;

        {
            // Lock removed
            EnsureWatchpointWriteHook();
            bool hasBreakpoints = breakpointManager.HasAnyBreakpointsFast();
            bool hasComplex = breakpointManager.HasComplexBreakpoints();
            bool hooks = bus.HasActiveHooks() || hasBreakpoints;
            stepsInBatch = this->ProcessBatch(currentBatchSize, hooks, hasBreakpoints, hasComplex, batchInstructions);
        }

        totalExecuted += stepsInBatch;
        remaining -= stepsInBatch;

        if (stepsInBatch < currentBatchSize) {
            break;
        }

        if (instructionsPerSlice > MAX_BATCH_SIZE) {
            std::this_thread::yield();
        }
    }

    return totalExecuted;
}

int Emulator::ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions) {
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

            if (res != 0) {
                totalCycles.fetch_add(i + 1, std::memory_order_relaxed);
                totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                handleStop(res);
                return i + 1;
            }

            if (hasBreakpoints) [[unlikely]] {
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        totalCycles.fetch_add(i + 1, std::memory_order_relaxed);
                        totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                        handleStop(0);
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

            if (res != 0) {
                totalCycles.fetch_add(i + 1, std::memory_order_relaxed);
                totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                handleStop(res);
                return i + 1;
            }

            if (hasBreakpoints) [[unlikely]] {
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        totalCycles.fetch_add(i + 1, std::memory_order_relaxed);
                        totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                        handleStop(0);
                        return i + 1;
                    }
                }
            }
        }
    }
    
    totalCycles.fetch_add(count, std::memory_order_relaxed);
    totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
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
                        if (Init(currentBinPath, errorMsg)) {
                            Console::Clear();
                        } else {
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
