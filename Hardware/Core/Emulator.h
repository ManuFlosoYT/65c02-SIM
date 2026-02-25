#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "Hardware/Audio/SID.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Comm/ACIA.h"
#include "Hardware/Comm/VIA.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"
#include "Hardware/Memory/ROM.h"
#include "Hardware/Video/GPU.h"
#include "Hardware/Video/LCD.h"

namespace Core {

enum class SavestateLoadResult {
    Success,
    HashMismatch,
    VersionMismatch,
    StructuralError,
    GenericError
};

using namespace Hardware;

class Emulator {
public:
    Emulator();
    ~Emulator() = default;

    bool Init(const std::string& binPath, std::string& errorMsg);

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    int Step();
    void InjectKey(char c);

    void SetOutputCallback(std::function<void(char)> cb);
    void SetLCDOutputCallback(std::function<void(char)> cb) {
        lcd.SetOutputCallback(cb);
    }

    const char (&GetLCDScreen() const)[2][16] { return lcd.GetScreen(); }
    const LCD& GetLCD() const { return lcd; }
    const CPU& GetCPU() const { return cpu; }
    CPU& GetCPU() { return cpu; }
    const Bus& GetMem() const { return bus; }
    Bus& GetMem() { return bus; }
    GPU& GetGPU() { return gpu; }
    void SetGPUEnabled(bool enabled) { gpuEnabled = enabled; }
    bool IsGPUEnabled() const { return gpuEnabled; }

    SID& GetSID() { return sid; }
    VIA& GetVIA() { return via; }

    void SetCycleAccurate(bool enabled) { cpu.SetCycleAccurate(enabled); }
    bool IsCycleAccurate() const { return cpu.IsCycleAccurate(); }

    void PrintState();

    // Threading
    void Start();
    void Stop();
    void Pause();
    void Resume();
    bool IsRunning() const { return running; }
    bool IsPaused() const { return paused; }

    void SetTargetIPS(int ips) { targetIPS = ips; }
    int GetTargetIPS() const { return targetIPS; }
    int GetActualIPS() const { return actualIPS; }

    void SetProfilingEnabled(bool enabled) { bus.SetProfilingEnabled(enabled); }
    void ClearProfiler() { bus.ClearProfiler(); }
    uint32_t* GetProfilerCounts() { return bus.GetProfilerCounts(); }

    void SetAutoReload(bool enabled) { autoReloadRequested = enabled; }
    bool IsAutoReloadEnabled() const { return autoReloadRequested; }

    SavestateLoadResult GetLastLoadResult() const { return lastLoadResult; }
    std::string GetLastLoadVersion() const { return lastLoadVersion; }

private:
    void ThreadLoop();

    // Components
    Bus bus;
    CPU cpu;
    LCD lcd;
    RAM ram;
    ROM rom;
    ACIA acia;
    GPU gpu;
    SID sid;
    VIA via;

    // Input buffer
    std::deque<char> inputBuffer;
    std::mutex bufferMutex;
    std::atomic<bool> hasInput{false};
    int baudDelay = 0;
    bool gpuEnabled = false;

    // Threading control
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::atomic<int> targetIPS{1000000};
    std::atomic<int> actualIPS{0};
    std::thread emulatorThread;
    std::mutex threadMutex;
    std::condition_variable pauseCV;
    std::mutex emulationMutex;  // mutex for thread safety during reset/step

    std::string currentBinPath;
    std::filesystem::file_time_type lastBinModificationTime;
    std::atomic<bool> autoReloadRequested{true};

    SavestateLoadResult lastLoadResult = SavestateLoadResult::Success;
    std::string lastLoadVersion;
};

}  // namespace Core
