#pragma once
#include <array>
#include <atomic>
#include <chrono>
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

struct SnapshotDelta {
    struct Chunk {
        uint32_t offset = 0;
        std::vector<uint8_t> xorData;
    };
    std::vector<Chunk> chunks;
    std::string fullState;
    bool isKeyframe = false;
    uint64_t totalCycles = 0;
    uint64_t totalInstructions = 0;
};

using namespace Hardware;

class Emulator {
   public:
    struct RuntimeStats {
        uint64_t cyclesPerSecond = 0;
        uint64_t cpuCyclesPerSecond = 0;
        uint64_t gpuCyclesPerSecond = 0;
        uint64_t instructionsPerSecond = 0;
        uint64_t slicesPerSecond = 0;
        uint64_t emulationNanoseconds = 0;
        uint64_t sleepNanoseconds = 0;
        uint64_t lateNanoseconds = 0;
    };

    Emulator();
    ~Emulator() { Stop(); }

    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;

    bool Init(const std::string& binPath, std::string& errorMsg);
    bool InitFromMemory(std::span<const uint8_t> data, const std::string& name, std::string& errorMsg);
    void Reset();

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    void Rewind();
    bool CanRewind() const;
    void StepBack();

    int Step();
    template <bool Debug>
    int Step();

    void TriggerIRQ();
    void TriggerNMI();

    void InjectKey(char key);

    void SetOutputCallback(std::function<void(char)> callback);
    const LCD& GetLCD() const;
    const CPU& GetCPU() const;
    CPU& GetCPU();
    const Bus& GetMem() const;
    Bus& GetMem();
    GPU& GetGPU();
    void SetGPUEnabled(bool enabled);
    bool IsGPUEnabled() const;
    void SetRasterBeamSlowMotion(bool enabled);
    [[nodiscard]] bool IsRasterBeamSlowMotion() const;
    void SetSIDEnabled(bool enabled);

    void SetESPEnabled(bool enabled);
    bool IsESPEnabled() const;

    void SetSDEnabled(bool enabled);
    bool IsSDEnabled() const;

    Hardware::ESP8266& GetESP8266();
    const Hardware::ESP8266& GetESP8266() const;
    Hardware::ACIA& GetACIA();
    const Hardware::ACIA& GetACIA() const;
    void SetACIAESPBridge(bool enabled);
    [[nodiscard]] bool IsACIAESPBridge() const;

    Hardware::SID& GetSID();
    const Hardware::SID& GetSID() const;
    [[nodiscard]] bool IsSIDEnabled() const { return sidEnabled; }
    Hardware::VIA& GetVIA();
    Hardware::ROM& GetROM();
    Hardware::SDCard& GetSDCard();
    const Hardware::SDCard& GetSDCard() const;
    Hardware::ScriptEngine& GetScriptEngine();
    Hardware::BreakpointManager& GetBreakpointManager();

    void SetCycleAccurate(bool enabled);
    bool IsCycleAccurate() const;

    // Threading
    void Start();
    void Stop();
    void Pause();
    void Resume();
    void WaitUntilSafeToMutate();
    bool IsRunning() const;
    bool IsPaused() const;
    bool IsHalted() const;

    [[nodiscard]] uint64_t GetTotalCycles() const { return totalCycles; }
    [[nodiscard]] uint64_t GetTotalInstructions() const { return totalInstructions; }
    void SetTargetIPS(int ips);
    int GetTargetIPS() const;
    [[nodiscard]] RuntimeStats GetRuntimeStats() const;

    void SetProfilingEnabled(bool enabled);
    void ClearProfiler();
    uint32_t* GetProfilerCounts();
    uint32_t* GetProfilerReadCounts();
    uint32_t* GetProfilerWriteCounts();

    void SetHeatmapEnabled(bool enabled);
    void ClearHeatmap();
    void DecayHeatmap(uint8_t amount);

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
    void SetRewindSnapshotsEnabled(bool enabled);
    void SetRewindInterval(int n);
    [[nodiscard]] int GetRewindInterval() const;
    void ClearRewindBuffer();

   private:
    void ThreadLoop();

    std::atomic<bool> pendingIRQ{false};
    std::atomic<bool> pendingNMI{false};
    std::atomic<bool> pendingInterruptAny{false};

    void HandleVIAPortB(Byte val);
    void RegisterCartridgeDevice(const DeviceConfig& dev, bool& sdCustomMapped);
    bool RegisterCartridgeLayout();
    void RegisterDefaultLayout();
    void ResetHardwareDevices();
    void LoadCartridgeVRAMIfPresent();
    void MountSDCardIfPresent();
    void EnsureWatchpointWriteHook();
    
    template <bool Debug, bool CycleAccurateMode>
    void RunCPUTick(int& res, bool& cpuStepped);
    void SyncHardwareCycles(bool cpuStepped, bool isNewInstruction, int ips);
    template <bool Debug, bool CycleAccurateMode>
    int StepInternal(int ips);
    template <bool Debug>
    void HandleInterrupts();
    template <bool Debug>
    void HandleSerialInput();


    bool SaveComponentsState(std::ostream& stateStream);
    void SaveInternalState(std::ostream& stateStream);
    bool LoadComponentsState(std::istream& stateStream);
    bool LoadInternalState(std::istream& stateStream);

    struct SliceResult {
        uint64_t cycles = 0;
        uint64_t cpuCycles = 0;
        uint64_t gpuCycles = 0;
        uint64_t instructions = 0;
    };

    SliceResult EmulateSlice(int instructionsPerSlice);
    int ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                     uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode);
    void handleStop(int code);
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
    bool watchpointWriteHookInstalled = false;

    ConsoleSerializable consoleSerializable;
    std::vector<ISerializable*> components;

    // Input buffer
    std::deque<char> inputBuffer;
    std::mutex bufferMutex;
    std::atomic<bool> hasInput{false};
    int baudDelay = 0;
    bool gpuEnabled = false;
    bool sidEnabled = true;
    bool sidEnabledOverride = false;
    bool espEnabled = false;
    bool sdEnabled = false;
    bool aciaEspBridge = false;
    bool rewindSnapshotsEnabled = true;
    int rewindSliceCounter = 0;

    // Threading control
    std::atomic<bool> running{false};
    std::atomic<bool> paused{true};
    std::atomic<bool> halted{false};
    std::atomic<int> targetIPS{1000000};
    std::atomic<uint64_t> actualCyclesPerSecond{0};
    std::atomic<uint64_t> actualCpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualGpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualInstructionsPerSecond{0};
    std::atomic<uint64_t> slicesPerSecond{0};
    std::atomic<uint64_t> emulationNanoseconds{0};
    std::atomic<uint64_t> sleepNanoseconds{0};
    std::atomic<uint64_t> lateNanoseconds{0};
    uint64_t lastStepCpuCycles = 0;
    uint64_t lastStepGpuCycles = 0;
    std::thread emulatorThread;
    std::mutex threadMutex;
    std::condition_variable pauseCV;
    std::atomic<bool> isActuallyPaused{true};
    std::atomic<uint64_t> totalCycles{0};
    std::atomic<uint64_t> totalInstructions{0};
    uint64_t totalCyclesAtLastResume{0};

    std::string currentBinPath;
    std::filesystem::file_time_type lastBinModificationTime;
    std::atomic<bool> autoReloadRequested{true};
    std::atomic<bool> rasterBeamSlowMotion{false};

    // Savestate versioning
    SavestateLoadResult lastLoadResult = SavestateLoadResult::Success;
    std::string lastLoadVersion;

    std::deque<SnapshotDelta> rewindBuffer;
    std::string lastCapturedState;
    uint64_t lastCapturedCycles = 0;
    uint64_t lastCapturedInstructions = 0;
    mutable std::mutex rewindMutex;
    static constexpr size_t MAX_REWIND_STATES = 600;
    int rewindInterval = 1;

    Cartridge cartridge;
};

}  // namespace Core

inline const Hardware::LCD& Core::Emulator::GetLCD() const { return lcd; }
inline const Hardware::CPU& Core::Emulator::GetCPU() const { return cpu; }
inline Hardware::CPU& Core::Emulator::GetCPU() { return cpu; }
inline const Hardware::Bus& Core::Emulator::GetMem() const { return bus; }
inline Hardware::Bus& Core::Emulator::GetMem() { return bus; }
inline Hardware::GPU& Core::Emulator::GetGPU() { return gpu; }
inline void Core::Emulator::SetGPUEnabled(bool enabled) { 
    if (gpuEnabled == enabled) { return; }
    gpuEnabled = enabled; 
    SetupHardware();
}
inline bool Core::Emulator::IsGPUEnabled() const { return gpuEnabled; }
inline void Core::Emulator::SetRasterBeamSlowMotion(bool enabled) {
    rasterBeamSlowMotion.store(enabled, std::memory_order_relaxed);
}
inline bool Core::Emulator::IsRasterBeamSlowMotion() const {
    return rasterBeamSlowMotion.load(std::memory_order_relaxed);
}

inline void Core::Emulator::SetESPEnabled(bool enabled) {
    if (espEnabled == enabled) { return; }
    espEnabled = enabled;
    SetupHardware();
}
inline bool Core::Emulator::IsESPEnabled() const { return espEnabled; }
inline Hardware::ESP8266& Core::Emulator::GetESP8266() { return esp8266; }
inline const Hardware::ESP8266& Core::Emulator::GetESP8266() const { return esp8266; }
inline Hardware::ACIA& Core::Emulator::GetACIA() { return acia; }
inline const Hardware::ACIA& Core::Emulator::GetACIA() const { return acia; }
inline void Core::Emulator::SetACIAESPBridge(bool enabled) { aciaEspBridge = enabled; }
inline bool Core::Emulator::IsACIAESPBridge() const { return aciaEspBridge; }

inline void Core::Emulator::SetSDEnabled(bool enabled) {
    if (sdEnabled == enabled) { return; }
    sdEnabled = enabled;
    SetupHardware();
}
inline bool Core::Emulator::IsSDEnabled() const { return sdEnabled; }

inline Hardware::SID& Core::Emulator::GetSID() { return sid; }
inline const Hardware::SID& Core::Emulator::GetSID() const { return sid; }
inline Hardware::VIA& Core::Emulator::GetVIA() { return via; }
inline Hardware::ROM& Core::Emulator::GetROM() { return rom; }
inline Hardware::SDCard& Core::Emulator::GetSDCard() { return sdcard; }
inline const Hardware::SDCard& Core::Emulator::GetSDCard() const { return sdcard; }
inline Hardware::ScriptEngine& Core::Emulator::GetScriptEngine() { return scriptEngine; }
inline Hardware::BreakpointManager& Core::Emulator::GetBreakpointManager() { return breakpointManager; }

inline void Core::Emulator::SetCycleAccurate(bool enabled) { cpu.SetCycleAccurate(enabled); }
inline bool Core::Emulator::IsCycleAccurate() const { return cpu.IsCycleAccurate(); }

inline bool Core::Emulator::IsRunning() const { return running; }
inline bool Core::Emulator::IsPaused() const { return paused; }
inline bool Core::Emulator::IsHalted() const { return halted; }

inline void Core::Emulator::SetTargetIPS(int ips) { targetIPS = ips; }
inline int Core::Emulator::GetTargetIPS() const { return targetIPS; }
inline Core::Emulator::RuntimeStats Core::Emulator::GetRuntimeStats() const {
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

inline void Core::Emulator::SetProfilingEnabled(bool enabled) { bus.SetProfilingEnabled(enabled); }
inline void Core::Emulator::ClearProfiler() { bus.ClearProfiler(); }
inline uint32_t* Core::Emulator::GetProfilerCounts() { return bus.GetProfilerCounts(); }
inline uint32_t* Core::Emulator::GetProfilerReadCounts() { return bus.GetProfilerReadCounts(); }
inline uint32_t* Core::Emulator::GetProfilerWriteCounts() { return bus.GetProfilerWriteCounts(); }

inline void Core::Emulator::SetHeatmapEnabled(bool enabled) { bus.SetHeatmapEnabled(enabled); }
inline void Core::Emulator::ClearHeatmap() { bus.ClearHeatmap(); }
inline void Core::Emulator::DecayHeatmap(uint8_t amount) { bus.DecayHeatmap(amount); }

inline void Core::Emulator::SetAutoReload(bool enabled) { autoReloadRequested = enabled; }
inline bool Core::Emulator::IsAutoReloadEnabled() const { return autoReloadRequested; }

inline Core::SavestateLoadResult Core::Emulator::GetLastLoadResult() const { return lastLoadResult; }
inline std::string Core::Emulator::GetLastLoadVersion() const { return lastLoadVersion; }
inline std::string Core::Emulator::GetCurrentBinPath() const { return currentBinPath; }

inline void Core::Emulator::StepBack() { Rewind(); }

inline void Core::Emulator::SetRewindInterval(int n) {
    if (n > 0) {
        rewindInterval = n;
    }
}

inline int Core::Emulator::GetRewindInterval() const { return rewindInterval; }

inline void Core::Emulator::ClearRewindBuffer() {
    std::scoped_lock lock(rewindMutex);
    rewindBuffer.clear();
    lastCapturedState.clear();
    lastCapturedCycles = 0;
    lastCapturedInstructions = 0;
    rewindSliceCounter = 0;
}

inline void Core::Emulator::SetRewindSnapshotsEnabled(bool enabled) {
    rewindSnapshotsEnabled = enabled;
    if (!rewindSnapshotsEnabled) {
        ClearRewindBuffer();
    }
}
