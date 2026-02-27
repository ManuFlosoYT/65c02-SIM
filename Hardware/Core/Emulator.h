#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
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

enum class SavestateLoadResult : std::uint8_t { Success, HashMismatch, VersionMismatch, StructuralError, GenericError };

using namespace Hardware;

class Emulator {
   public:
    Emulator();
    ~Emulator() = default;

    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;

    bool Init(const std::string& binPath, std::string& errorMsg);

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    int Step();
    template <bool Debug>
    int Step();
    void InjectKey(char key);

    void SetOutputCallback(std::function<void(char)> callback);
    void SetLCDOutputCallback(std::function<void(char)> callback);

    const std::array<std::array<char, 16>, 2>& GetLCDScreen() const;
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
    ROM& GetROM();

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

    bool LoadComponentsState(std::istream& stateStream);
    void LoadInternalState(std::istream& stateStream);

    void EmulateSlice(int instructionsPerSlice);
    void CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck);

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

inline void Core::Emulator::SetLCDOutputCallback(std::function<void(char)> callback) {
    lcd.SetOutputCallback(std::move(callback));
}

inline const std::array<std::array<char, 16>, 2>& Core::Emulator::GetLCDScreen() const { return lcd.GetScreen(); }
inline const Hardware::LCD& Core::Emulator::GetLCD() const { return lcd; }
inline const Hardware::CPU& Core::Emulator::GetCPU() const { return cpu; }
inline Hardware::CPU& Core::Emulator::GetCPU() { return cpu; }
inline const Hardware::Bus& Core::Emulator::GetMem() const { return bus; }
inline Hardware::Bus& Core::Emulator::GetMem() { return bus; }
inline Hardware::GPU& Core::Emulator::GetGPU() { return gpu; }
inline void Core::Emulator::SetGPUEnabled(bool enabled) { gpuEnabled = enabled; }
inline bool Core::Emulator::IsGPUEnabled() const { return gpuEnabled; }

inline Hardware::SID& Core::Emulator::GetSID() { return sid; }
inline Hardware::VIA& Core::Emulator::GetVIA() { return via; }
inline Hardware::ROM& Core::Emulator::GetROM() { return rom; }

inline void Core::Emulator::SetCycleAccurate(bool enabled) { cpu.SetCycleAccurate(enabled); }
inline bool Core::Emulator::IsCycleAccurate() const { return cpu.IsCycleAccurate(); }

inline bool Core::Emulator::IsRunning() const { return running; }
inline bool Core::Emulator::IsPaused() const { return paused; }

inline void Core::Emulator::SetTargetIPS(int ips) { targetIPS = ips; }
inline int Core::Emulator::GetTargetIPS() const { return targetIPS; }
inline int Core::Emulator::GetActualIPS() const { return actualIPS; }

inline void Core::Emulator::SetProfilingEnabled(bool enabled) { bus.SetProfilingEnabled(enabled); }
inline void Core::Emulator::ClearProfiler() { bus.ClearProfiler(); }
inline uint32_t* Core::Emulator::GetProfilerCounts() { return bus.GetProfilerCounts(); }

inline void Core::Emulator::SetAutoReload(bool enabled) { autoReloadRequested = enabled; }
inline bool Core::Emulator::IsAutoReloadEnabled() const { return autoReloadRequested; }

inline Core::SavestateLoadResult Core::Emulator::GetLastLoadResult() const { return lastLoadResult; }
inline std::string Core::Emulator::GetLastLoadVersion() const { return lastLoadVersion; }
inline std::string Core::Emulator::GetCurrentBinPath() const { return currentBinPath; }
