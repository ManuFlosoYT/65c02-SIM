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
#include <string_view>
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
#include "Backend/IEmulatorBackend.h"

namespace Core {

using namespace Hardware;

class Emulator : public Backend::IEmulatorBackend {
   public:
    friend class EmulationEngine;
    friend class SavestateManager;

    using RuntimeStats = EmulationEngine::RuntimeStats;

    Emulator();
    ~Emulator() override { Stop(); }

    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;

    [[nodiscard]] bool HasError() const noexcept override { return hasError; }
    [[nodiscard]] const std::string& GetLastError() const noexcept override { return lastError; }
    void SetError(std::string_view msg) override {
        hasError = true;
        lastError = std::string(msg);
    }
    void ClearError() noexcept override {
        hasError = false;
        lastError.clear();
    }

    bool Init(const std::string& binPath, std::string& errorMsg) override;
    bool InitFromMemory(std::span<const uint8_t> data, const std::string& name, std::string& errorMsg) override;
    void Reset() override;

    bool SaveState(const std::string& filename) override;
    bool LoadState(const std::string& filename, bool forceLoad = false) override;

    void Rewind() override;
    [[nodiscard]] bool CanRewind() const override;
    void StepBack() override;

    int Step() override;
    template <bool Debug>
    int Step();

    void TriggerIRQ();
    void TriggerNMI();

    void InjectKey(char key) override;
    void InjectText(std::string_view text) override;

    void SetOutputCallback(std::function<void(char)> callback) override;
    [[nodiscard]] const LCD& GetLCD() const override;
    [[nodiscard]] const CPU& GetCPU() const override;
    CPU& GetCPU() override;
    [[nodiscard]] const Bus& GetMem() const override;
    Bus& GetMem() override;
    [[nodiscard]] const GPU& GetGPU() const override;
    GPU& GetGPU() override;
    Hardware::MicroDosGPU& GetMicroDosGPU() override;
    [[nodiscard]] const Hardware::MicroDosGPU& GetMicroDosGPU() const override;
    void SetGPUEnabled(bool enabled) override;
    [[nodiscard]] bool IsGPUEnabled() const override;
    void SetRasterBeamSlowMotion(bool enabled) override;
    [[nodiscard]] bool IsRasterBeamSlowMotion() const override;
    void SetSIDEnabled(bool enabled) override;

    void SetESPEnabled(bool enabled) override;
    [[nodiscard]] bool IsESPEnabled() const override;

    void SetSDEnabled(bool enabled) override;
    [[nodiscard]] bool IsSDEnabled() const override;

    Hardware::ESP8266& GetESP8266() override;
    [[nodiscard]] const Hardware::ESP8266& GetESP8266() const override;
    Hardware::ACIA& GetACIA();
    const Hardware::ACIA& GetACIA() const;
    void SetACIAESPBridge(bool enabled) override;
    [[nodiscard]] bool IsACIAESPBridge() const override;

    Hardware::SID& GetSID() override;
    [[nodiscard]] const Hardware::SID& GetSID() const override;
    [[nodiscard]] bool IsSIDEnabled() const override { return sidEnabled; }
    Hardware::VIA& GetVIA();
    Hardware::ROM& GetROM() override;
    Hardware::SDCard& GetSDCard() override;
    Hardware::RAM& GetRAM() override { return ram; }
    [[nodiscard]] const Hardware::SDCard& GetSDCard() const override;
    Hardware::ScriptEngine& GetScriptEngine() override;
    Hardware::BreakpointManager& GetBreakpointManager() override;

    void SetLogicAnalyzerEnabled(bool enabled) override;
    [[nodiscard]] bool IsLogicAnalyzerEnabled() const override;
    Hardware::LogicAnalyzer& GetLogicAnalyzer() override;
    [[nodiscard]] const Hardware::LogicAnalyzer& GetLogicAnalyzer() const override;
    void SampleLogicAnalyzer(Word addr, Byte data, bool rw, bool sync);

    void SetCycleAccurate(bool enabled) override;
    [[nodiscard]] bool IsCycleAccurate() const override;

    void Start() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void WaitUntilSafeToMutate() override;
    [[nodiscard]] bool IsRunning() const override;
    [[nodiscard]] bool IsPaused() const override;
    [[nodiscard]] bool IsHalted() const override;

    [[nodiscard]] uint64_t GetTotalCycles() const override { return totalCycles; }
    [[nodiscard]] uint64_t GetTotalInstructions() const { return totalInstructions; }
    void SetTargetIPS(int ips) override;
    [[nodiscard]] int GetTargetIPS() const override;
    [[nodiscard]] RuntimeStats GetRuntimeStats() const override;

    void SetProfilingEnabled(bool enabled) override;
    void ClearProfiler() override;
    Hardware::SubroutineProfiler& GetSubroutineProfiler() override;
    [[nodiscard]] const Hardware::SubroutineProfiler& GetSubroutineProfiler() const override;
    uint32_t* GetProfilerCounts() override;
    uint32_t* GetProfilerReadCounts() override;
    uint32_t* GetProfilerWriteCounts() override;

    void SetHeatmapEnabled(bool enabled) override;
    void ClearHeatmap() override;
    void DecayHeatmap(uint8_t amount) override;

    void SetAutoReload(bool enabled) override;
    [[nodiscard]] bool IsAutoReloadEnabled() const override;

    [[nodiscard]] SavestateLoadResult GetLastLoadResult() const override;
    [[nodiscard]] std::string GetLastLoadVersion() const override;
    [[nodiscard]] std::string GetCurrentBinPath() const override;

    void SetCartridge(const Cartridge& cart) override { cartridge = cart; }
    [[nodiscard]] const Cartridge& GetCartridge() const override { return cartridge; }
    Cartridge& GetCartridge() override { return cartridge; }
    void ClearCartridge() override { cartridge = Cartridge(); }

    void SetupHardware();
    void SetRewindSnapshotsEnabled(bool enabled) override;
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
    bool hasError = false;
    std::string lastError;

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
