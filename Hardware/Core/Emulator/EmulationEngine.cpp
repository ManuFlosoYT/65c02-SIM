#include "Hardware/Core/Emulator.h"

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

void Emulator::ThreadLoop() {
    using namespace std::chrono;
    auto nextSliceTime = high_resolution_clock::now();
    int instructionsThisSecond = 0;
    auto lastSecondTime = high_resolution_clock::now();

    double instructionAccumulator = 0.0;
    auto lastWatchCheck = high_resolution_clock::now();

    while (running.load(std::memory_order_relaxed)) {
        if (paused.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
        }
        if (!running.load(std::memory_order_relaxed)) {
            break;
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

    bool hooks = bus.HasActiveHooks() || breakpointManager.HasAnyBreakpointsFast();
    const int MAX_BATCH_SIZE = 100000;
    int remaining = instructionsPerSlice;
    int totalExecuted = 0;

    // Save state once per slice (50ms interval)
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSaveTime).count() >= 50) {
        std::lock_guard<std::recursive_mutex> lock(emulationMutex);
        SaveStateToBuffer();
        lastSaveTime = now;
    }

    // Use safety lock only if Python API or active hooks/breakpoints require it
    bool safetyRequired = scriptEngine.IsScriptRunning() || hooks;

    bool hasBreakpoints = breakpointManager.HasAnyBreakpointsFast();

    while (remaining > 0) {
        int currentBatch = std::min(remaining, MAX_BATCH_SIZE);
        {
            // Conditional lock: only acquire if safety is required (unlikely for hot path)
            std::unique_lock<std::recursive_mutex> lock(emulationMutex, std::defer_lock);
            if (safetyRequired) [[unlikely]] {
                lock.lock();
            }

            int batchInstructions = 0;
            for (int i = 0; i < currentBatch; ++i) {
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
                    totalCycles.fetch_add(i+1, std::memory_order_relaxed);
                    totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                    handleStop(res);
                    return totalExecuted;
                }
                
                // ONLY evaluate if there are actual breakpoints active
                if (hasBreakpoints && breakpointManager.Evaluate(cpu, bus) > 0) {
                    totalCycles.fetch_add(i+1, std::memory_order_relaxed);
                    totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
                    handleStop(0);
                    return totalExecuted;
                }
                totalExecuted++;
            }
            totalCycles.fetch_add(currentBatch, std::memory_order_relaxed);
            totalInstructions.fetch_add(batchInstructions, std::memory_order_relaxed);
        }
        remaining -= currentBatch;
        
        // At high frequencies, yield between batches to let UI thread catch the mutex
        if (instructionsPerSlice > MAX_BATCH_SIZE) {
            std::this_thread::yield();
        }
    }

    return totalExecuted;
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
