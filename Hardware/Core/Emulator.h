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
#include "Hardware/Comm/SDCard.h"
#include "Hardware/Comm/VIA.h"
#include "Hardware/Comm/ESP8266.h"
#include "Hardware/Scripting/ScriptEngine.h"
#include "Hardware/Core/BreakpointManager.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"
#include "Hardware/Memory/ROM.h"
#include "Hardware/Video/GPU.h"
#include "Hardware/Video/LCD.h"
#include "Hardware/Core/ISerializable.h"
#include "Hardware/Core/Cartridge.h"
#include "Frontend/Control/Console.h"

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
    bool InitFromMemory(const uint8_t* data, size_t size, const std::string& name, std::string& errorMsg);
    void Reset();

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    void Rewind();
    bool CanRewind() const;

    int Step();
    template <bool Debug>
    int Step();

    void TriggerIRQ();
    void TriggerNMI();

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

    void SetESPEnabled(bool enabled);
    bool IsESPEnabled() const;

    void SetSDEnabled(bool enabled);
    bool IsSDEnabled() const;

    Hardware::SID& GetSID();
    const Hardware::SID& GetSID() const;
    Hardware::VIA& GetVIA();
    Hardware::ROM& GetROM();
    Hardware::SDCard& GetSDCard();
    Hardware::ESP8266& GetESP8266();
    Hardware::ScriptEngine& GetScriptEngine();
    Hardware::BreakpointManager& GetBreakpointManager();
    std::recursive_mutex& GetMutex() { return emulationMutex; }

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
    bool IsHalted() const;

    [[nodiscard]] uint64_t GetTotalCycles() const { return totalCycles; }
    [[nodiscard]] uint64_t GetTotalInstructions() const { return totalInstructions; }
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

    void SetCartridge(const Cartridge& cart) { cartridge = cart; }
    const Cartridge& GetCartridge() const { return cartridge; }
    Cartridge& GetCartridge() { return cartridge; }
    void ClearCartridge() { cartridge = Cartridge(); }

    void SetupHardware();
    void SetHeadless(bool isHeadless) { headless = isHeadless; }
    [[nodiscard]] bool IsHeadless() const { return headless; }

   private:
    void ThreadLoop();

    std::atomic<bool> pendingIRQ{false};
    std::atomic<bool> pendingNMI{false};

    void HandleVIAPortB(Byte val);

    bool LoadComponentsState(std::istream& stateStream);
    void LoadInternalState(std::istream& stateStream);

    void EmulateSlice(int instructionsPerSlice);
    void CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck);

    void SaveStateToBuffer();

    struct ConsoleSerializable : public ISerializable {
        bool SaveState(std::ostream& out) const override { return Console::SaveState(out); }
        bool LoadState(std::istream& inStream) override { return Console::LoadState(inStream); }
    };

    // Components
    Bus bus;
    CPU cpu;
    LCD lcd;
    RAM ram;
    ROM rom;
    ACIA acia;
    GPU gpu;
    Hardware::SID sid;
    Hardware::VIA via;
    Hardware::SDCard sdcard;
    Hardware::ESP8266 esp8266;
    Hardware::ScriptEngine scriptEngine;
    Hardware::BreakpointManager breakpointManager;

    ConsoleSerializable consoleSerializable;
    std::vector<ISerializable*> components;

    // Input buffer
    std::deque<char> inputBuffer;
    std::mutex bufferMutex;
    std::atomic<bool> hasInput{false};
    int baudDelay = 0;
    bool gpuEnabled = false;
    bool espEnabled = false;
    bool sdEnabled = false;
    bool headless = false;

    // Threading control
    std::atomic<bool> running{false};
    std::atomic<bool> paused{true};
    std::atomic<bool> halted{false};
    std::atomic<int> targetIPS{1000000};
    std::atomic<int> actualIPS{0};
    std::thread emulatorThread;
    std::mutex threadMutex;
    std::condition_variable pauseCV;
    mutable std::recursive_mutex emulationMutex;  // mutex for thread safety during reset/step (recursive to support re-entrancy from script hooks)
    std::atomic<uint64_t> totalCycles{0};
    std::atomic<uint64_t> totalInstructions{0};
    uint64_t totalCyclesAtLastResume{0};

    std::string currentBinPath;
    std::filesystem::file_time_type lastBinModificationTime;
    std::atomic<bool> autoReloadRequested{true};

    // Savestate versioning
    SavestateLoadResult lastLoadResult = SavestateLoadResult::Success;
    std::string lastLoadVersion;

    std::deque<std::string> rewindBuffer;
    static constexpr size_t MAX_REWIND_STATES = 255;

    Cartridge cartridge;
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
inline void Core::Emulator::SetGPUEnabled(bool enabled) { 
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    gpuEnabled = enabled; 
    SetupHardware();
}
inline bool Core::Emulator::IsGPUEnabled() const { return gpuEnabled; }

inline void Core::Emulator::SetESPEnabled(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    espEnabled = enabled;
    SetupHardware();
}
inline bool Core::Emulator::IsESPEnabled() const { return espEnabled; }

inline void Core::Emulator::SetSDEnabled(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    sdEnabled = enabled;
    SetupHardware();
}
inline bool Core::Emulator::IsSDEnabled() const { return sdEnabled; }

inline Hardware::SID& Core::Emulator::GetSID() { return sid; }
inline const Hardware::SID& Core::Emulator::GetSID() const { return sid; }
inline Hardware::VIA& Core::Emulator::GetVIA() { return via; }
inline Hardware::ROM& Core::Emulator::GetROM() { return rom; }
inline Hardware::SDCard& Core::Emulator::GetSDCard() { return sdcard; }
inline Hardware::ESP8266& Core::Emulator::GetESP8266() { return esp8266; }
inline Hardware::ScriptEngine& Core::Emulator::GetScriptEngine() { return scriptEngine; }
inline Hardware::BreakpointManager& Core::Emulator::GetBreakpointManager() { return breakpointManager; }

inline void Core::Emulator::SetCycleAccurate(bool enabled) { cpu.SetCycleAccurate(enabled); }
inline bool Core::Emulator::IsCycleAccurate() const { return cpu.IsCycleAccurate(); }

inline bool Core::Emulator::IsRunning() const { return running; }
inline bool Core::Emulator::IsPaused() const { return paused; }
inline bool Core::Emulator::IsHalted() const { return halted; }

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
