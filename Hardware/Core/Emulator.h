#pragma once
#include <atomic>
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
    template <bool Debug>
    int Step();
    void InjectKey(char c);

    void SetOutputCallback(std::function<void(char)> cb);
    void SetLCDOutputCallback(std::function<void(char)> cb);

    const char (&GetLCDScreen() const)[2][16];
    const LCD& GetLCD() const;
    const CPU& GetCPU() const;
    CPU& GetCPU();
    const Bus& GetMem() const;
    Bus& GetMem();
    GPU& GetGPU();
    void SetGPUEnabled(bool enabled);
    bool IsGPUEnabled() const;

    SID& GetSID();
    VIA& GetVIA();

    void SetCycleAccurate(bool enabled);
    bool IsCycleAccurate() const;

    void PrintState();

    // Threading
    void Start();
    void Stop();
    void Pause();
    void Resume();
    bool IsRunning() const;
    bool IsPaused() const;

    void SetTargetIPS(int ips);
    int GetTargetIPS() const;
    int GetActualIPS() const;

    void SetProfilingEnabled(bool enabled);
    void ClearProfiler();
    uint32_t* GetProfilerCounts();

    void SetAutoReload(bool enabled);
    bool IsAutoReloadEnabled() const;

    SavestateLoadResult GetLastLoadResult() const;
    std::string GetLastLoadVersion() const;
    std::string GetCurrentBinPath() const;

private:
    void ThreadLoop();
    void SetupHardware();

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

#include "Hardware/Core/Emulator.inl"
