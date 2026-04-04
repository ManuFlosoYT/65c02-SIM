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

        EmulateSlice(instructionsPerSlice);

        instructionsThisSecond += instructionsPerSlice;

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
        }

        CheckAutoReload(lastWatchCheck);
    }
}

void Emulator::EmulateSlice(int instructionsPerSlice) {
    bool hooks = bus.HasActiveHooks() || breakpointManager.HasActiveBreakpoints();
    std::lock_guard<std::mutex> lock(emulationMutex);
    SaveStateToBuffer();

    auto runStep = [this](bool useHooks) -> int {
        int res = useHooks ? Step<true>() : Step<false>();
        if (res != 0) {
            Pause();
#ifndef TARGET_WASM
            if (totalCycles > totalCyclesAtLastResume) {
                sid.StopRecording();
            }
#endif
            halted = true;
            std::cerr << "Emulator stopped with code: " << res << '\n';
        }
        return res;
    };

    for (int i = 0; i < instructionsPerSlice; ++i) {
        int res = runStep(hooks);
        if (res != 0) {
            break;
        }

        if (breakpointManager.HasActiveBreakpoints()) {
            uint16_t wpAddr = 0;
            breakpointManager.ConsumeWatchpointHit(wpAddr);

            uint32_t hitId = breakpointManager.Evaluate(cpu, bus);
            if (hitId > 0) {
                Pause();
                sid.SetEmulationPaused(true);
                break;
            }
        }
    }
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
