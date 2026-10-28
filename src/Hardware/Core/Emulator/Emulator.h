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
#include "Hardware/Storage/SDCard.h"
#include "Hardware/Comm/VIA.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"
#include "Scripting/ScriptEngine.h"
#include "Hardware/Core/BreakpointManager.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"
#include "Hardware/Memory/ROM.h"
#include "Hardware/Video/GPU.h"
#include "Hardware/Video/MicroDosGPU.h"
#include "Hardware/Video/LCD.h"
#include "Hardware/Core/ISerializable.h"
#include "Hardware/Core/Cartridge.h"
#include "Hardware/Core/LogicAnalyzer.h"
#include "Hardware/Core/SubroutineProfiler.h"
#include "Hardware/Core/Console.h"
#include "Hardware/Core/Emulator/SavestateManager.h"
#include "Hardware/Core/Emulator/EmulationEngine.h"
#include "Hardware/Core/Emulator/SerialIOBridge.h"
#include "Hardware/Core/Emulator/SystemConfigurator.h"

namespace Core {

using namespace Hardware;

class Emulator {
   public:
    friend class EmulationEngine;
    friend class SavestateManager;

    using RuntimeStats = EmulationEngine::RuntimeStats;

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
    const GPU& GetGPU() const;
    GPU& GetGPU();
    Hardware::MicroDosGPU& GetMicroDosGPU();
    const Hardware::MicroDosGPU& GetMicroDosGPU() const;
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
    Hardware::RAM& GetRAM() { return ram; }
    const Hardware::SDCard& GetSDCard() const;
    Hardware::ScriptEngine& GetScriptEngine();
    Hardware::BreakpointManager& GetBreakpointManager();

    void SetLogicAnalyzerEnabled(bool enabled);
    [[nodiscard]] bool IsLogicAnalyzerEnabled() const;
    Hardware::LogicAnalyzer& GetLogicAnalyzer();
    [[nodiscard]] const Hardware::LogicAnalyzer& GetLogicAnalyzer() const;
    void SampleLogicAnalyzer(Word addr, Byte data, bool rw, bool sync);

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
    Hardware::SubroutineProfiler& GetSubroutineProfiler();
    const Hardware::SubroutineProfiler& GetSubroutineProfiler() const;
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
    void SaveStateToBuffer();

    EmulationEngine& GetEngine() { return engine; }
    const EmulationEngine& GetEngine() const { return engine; }
    SavestateManager& GetSavestateManager() { return savestateManager; }
    const SavestateManager& GetSavestateManager() const { return savestateManager; }

    int ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                     uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode);
    void SaveInternalState(std::ostream& stateStream);
    bool LoadInternalState(std::istream& stateStream);

   private:
    friend class SystemConfigurator;
    friend class SavestateManager;
    friend class EmulationEngine;
    friend class BinaryFileWatcher;

    std::atomic<bool> pendingIRQ{false};
    std::atomic<bool> pendingNMI{false};
    std::atomic<bool> pendingInterruptAny{false};

    void HandleVIAPortB(Byte val);
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
    Hardware::MicroDosGPU microDosGpu;
    Hardware::GPU* activeGpu = &gpu;
    Hardware::SID sid;
    Hardware::VIA via;
    Hardware::SDCard sdcard;
    Hardware::ESP8266 esp8266;
    Hardware::ScriptEngine scriptEngine;
    Hardware::BreakpointManager breakpointManager;
    Hardware::LogicAnalyzer logicAnalyzer;
    Hardware::SubroutineProfiler subroutineProfiler;
    std::atomic<bool> logicAnalyzerEnabled{false};
    bool watchpointWriteHookInstalled = false;

    ConsoleSerializable consoleSerializable;
    std::vector<ISerializable*> components;

    SerialIOBridge serialBridge;
    bool gpuEnabled = false;
    bool sidEnabled = true;
    bool sidEnabledOverride = false;
    bool espEnabled = false;
    bool sdEnabled = false;

    std::atomic<uint64_t> totalCycles{0};
    std::atomic<uint64_t> totalInstructions{0};
    uint64_t lastStepCpuCycles = 0;
    uint64_t lastStepGpuCycles = 0;

    std::string currentBinPath;
    std::atomic<bool> rasterBeamSlowMotion{false};
    Cartridge cartridge;

    EmulationEngine engine;
    SavestateManager savestateManager;
};

}  // namespace Core

inline const Hardware::LCD& Core::Emulator::GetLCD() const { return lcd; }
inline const Hardware::CPU& Core::Emulator::GetCPU() const { return cpu; }
inline Hardware::CPU& Core::Emulator::GetCPU() { return cpu; }
inline const Hardware::Bus& Core::Emulator::GetMem() const { return bus; }
inline Hardware::Bus& Core::Emulator::GetMem() { return bus; }
inline const Hardware::GPU& Core::Emulator::GetGPU() const { return *activeGpu; }
inline Hardware::GPU& Core::Emulator::GetGPU() { return *activeGpu; }
inline Hardware::MicroDosGPU& Core::Emulator::GetMicroDosGPU() { return microDosGpu; }
inline const Hardware::MicroDosGPU& Core::Emulator::GetMicroDosGPU() const { return microDosGpu; }
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
inline void Core::Emulator::SetACIAESPBridge(bool enabled) { serialBridge.SetACIAESPBridge(enabled); }
inline bool Core::Emulator::IsACIAESPBridge() const { return serialBridge.IsACIAESPBridge(); }

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

inline void Core::Emulator::Start() { engine.Start(); }
inline void Core::Emulator::Stop() { engine.Stop(); }
inline void Core::Emulator::Pause() { engine.Pause(); }
inline void Core::Emulator::Resume() { engine.Resume(); }
inline void Core::Emulator::WaitUntilSafeToMutate() { engine.WaitUntilSafeToMutate(); }
inline bool Core::Emulator::IsRunning() const { return engine.IsRunning(); }
inline bool Core::Emulator::IsPaused() const { return engine.IsPaused(); }
inline bool Core::Emulator::IsHalted() const { return engine.IsHalted(); }

inline void Core::Emulator::SetTargetIPS(int ips) { engine.SetTargetIPS(ips); }
inline int Core::Emulator::GetTargetIPS() const { return engine.GetTargetIPS(); }
inline Core::Emulator::RuntimeStats Core::Emulator::GetRuntimeStats() const {
    return engine.GetRuntimeStats();
}

inline void Core::Emulator::SetProfilingEnabled(bool enabled) {
    bus.SetProfilingEnabled(enabled);
    subroutineProfiler.SetEnabled(enabled);
}
inline void Core::Emulator::ClearProfiler() {
    bus.ClearProfiler();
    subroutineProfiler.Clear();
}
inline Hardware::SubroutineProfiler& Core::Emulator::GetSubroutineProfiler() { return subroutineProfiler; }
inline const Hardware::SubroutineProfiler& Core::Emulator::GetSubroutineProfiler() const { return subroutineProfiler; }
inline uint32_t* Core::Emulator::GetProfilerCounts() { return bus.GetProfilerCounts(); }
inline uint32_t* Core::Emulator::GetProfilerReadCounts() { return bus.GetProfilerReadCounts(); }
inline uint32_t* Core::Emulator::GetProfilerWriteCounts() { return bus.GetProfilerWriteCounts(); }

inline void Core::Emulator::SetHeatmapEnabled(bool enabled) { bus.SetHeatmapEnabled(enabled); }
inline void Core::Emulator::ClearHeatmap() { bus.ClearHeatmap(); }
inline void Core::Emulator::DecayHeatmap(uint8_t amount) { bus.DecayHeatmap(amount); }

inline void Core::Emulator::SetAutoReload(bool enabled) { engine.SetAutoReload(enabled); }
inline bool Core::Emulator::IsAutoReloadEnabled() const { return engine.IsAutoReloadEnabled(); }

inline Core::SavestateLoadResult Core::Emulator::GetLastLoadResult() const { return savestateManager.GetLastLoadResult(); }
inline std::string Core::Emulator::GetLastLoadVersion() const { return savestateManager.GetLastLoadVersion(); }
inline std::string Core::Emulator::GetCurrentBinPath() const { return currentBinPath; }

inline bool Core::Emulator::SaveState(const std::string& filename) { return savestateManager.SaveState(filename); }
inline bool Core::Emulator::LoadState(const std::string& filename, bool forceLoad) { return savestateManager.LoadState(filename, forceLoad); }
inline void Core::Emulator::Rewind() { savestateManager.Rewind(); }
inline bool Core::Emulator::CanRewind() const { return savestateManager.CanRewind(); }
inline void Core::Emulator::StepBack() { savestateManager.StepBack(); }

inline void Core::Emulator::SetRewindInterval(int n) { savestateManager.SetRewindInterval(n); }
inline int Core::Emulator::GetRewindInterval() const { return savestateManager.GetRewindInterval(); }
inline void Core::Emulator::ClearRewindBuffer() { savestateManager.ClearRewindBuffer(); }
inline void Core::Emulator::SetRewindSnapshotsEnabled(bool enabled) { savestateManager.SetRewindSnapshotsEnabled(enabled); }
inline void Core::Emulator::SaveStateToBuffer() { savestateManager.SaveStateToBuffer(); }

inline void Core::Emulator::SetLogicAnalyzerEnabled(bool enabled) {
    logicAnalyzerEnabled.store(enabled, std::memory_order_relaxed);
}
inline bool Core::Emulator::IsLogicAnalyzerEnabled() const {
    return logicAnalyzerEnabled.load(std::memory_order_relaxed);
}
inline Hardware::LogicAnalyzer& Core::Emulator::GetLogicAnalyzer() {
    return logicAnalyzer;
}
inline const Hardware::LogicAnalyzer& Core::Emulator::GetLogicAnalyzer() const {
    return logicAnalyzer;
}
