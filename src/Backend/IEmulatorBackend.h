#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

#include "Hardware/Audio/SID.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Comm/ACIA.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"
#include "Hardware/Comm/VIA.h"
#include "Hardware/Core/BreakpointManager.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Core/Cartridge.h"
#include "Hardware/Core/Emulator/SavestateManager.h"
#include "Hardware/Core/Emulator/SchedulerTelemetry.h"
#include "Hardware/Core/LogicAnalyzer.h"
#include "Hardware/Core/SubroutineProfiler.h"
#include "Hardware/Memory/RAM.h"
#include "Hardware/Memory/ROM.h"
#include "Hardware/Storage/SDCard.h"
#include "Hardware/Video/GPU.h"
#include "Hardware/Video/LCD.h"
#include "Hardware/Video/MicroDosGPU.h"
#include "Scripting/ScriptEngine.h"

namespace Backend {

class IEmulatorBackend {
public:
    virtual ~IEmulatorBackend() = default;

    virtual bool Init(const std::string& binPath, std::string& errorMsg) = 0;
    virtual bool InitFromMemory(std::span<const uint8_t> data, const std::string& name, std::string& errorMsg) = 0;
    virtual void Reset() = 0;

    virtual bool SaveState(const std::string& filename) = 0;
    virtual bool LoadState(const std::string& filename, bool forceLoad = false) = 0;

    virtual void Rewind() = 0;
    [[nodiscard]] virtual bool CanRewind() const = 0;
    virtual void StepBack() = 0;
    virtual int Step() = 0;
    virtual void SetRewindSnapshotsEnabled(bool enabled) = 0;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void WaitUntilSafeToMutate() = 0;

    [[nodiscard]] virtual bool IsRunning() const = 0;
    [[nodiscard]] virtual bool IsPaused() const = 0;
    [[nodiscard]] virtual bool IsHalted() const = 0;

    virtual void InjectKey(char key) = 0;
    virtual void InjectText(std::string_view text) = 0;
    virtual void SetOutputCallback(std::function<void(char)> callback) = 0;

    [[nodiscard]] virtual const Hardware::LCD& GetLCD() const = 0;
    [[nodiscard]] virtual const Hardware::CPU& GetCPU() const = 0;
    virtual Hardware::CPU& GetCPU() = 0;
    [[nodiscard]] virtual const Hardware::Bus& GetMem() const = 0;
    virtual Hardware::Bus& GetMem() = 0;
    [[nodiscard]] virtual const Hardware::GPU& GetGPU() const = 0;
    virtual Hardware::GPU& GetGPU() = 0;
    virtual Hardware::MicroDosGPU& GetMicroDosGPU() = 0;
    [[nodiscard]] virtual const Hardware::MicroDosGPU& GetMicroDosGPU() const = 0;

    virtual void SetGPUEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsGPUEnabled() const = 0;
    virtual void SetRasterBeamSlowMotion(bool enabled) = 0;
    [[nodiscard]] virtual bool IsRasterBeamSlowMotion() const = 0;

    virtual void SetSIDEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsSIDEnabled() const = 0;

    virtual void SetESPEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsESPEnabled() const = 0;

    virtual void SetSDEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsSDEnabled() const = 0;

    virtual Hardware::ESP8266& GetESP8266() = 0;
    [[nodiscard]] virtual const Hardware::ESP8266& GetESP8266() const = 0;
    virtual Hardware::SID& GetSID() = 0;
    [[nodiscard]] virtual const Hardware::SID& GetSID() const = 0;
    virtual Hardware::ROM& GetROM() = 0;
    virtual Hardware::SDCard& GetSDCard() = 0;
    [[nodiscard]] virtual const Hardware::SDCard& GetSDCard() const = 0;
    virtual Hardware::RAM& GetRAM() = 0;
    virtual Hardware::ScriptEngine& GetScriptEngine() = 0;
    virtual Hardware::BreakpointManager& GetBreakpointManager() = 0;

    virtual void SetLogicAnalyzerEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsLogicAnalyzerEnabled() const = 0;
    virtual Hardware::LogicAnalyzer& GetLogicAnalyzer() = 0;
    [[nodiscard]] virtual const Hardware::LogicAnalyzer& GetLogicAnalyzer() const = 0;

    virtual void SetCycleAccurate(bool enabled) = 0;
    [[nodiscard]] virtual bool IsCycleAccurate() const = 0;

    [[nodiscard]] virtual uint64_t GetTotalCycles() const = 0;
    virtual void SetTargetIPS(int ips) = 0;
    [[nodiscard]] virtual int GetTargetIPS() const = 0;
    [[nodiscard]] virtual Core::RuntimeStats GetRuntimeStats() const = 0;

    virtual void SetProfilingEnabled(bool enabled) = 0;
    virtual void ClearProfiler() = 0;
    virtual Hardware::SubroutineProfiler& GetSubroutineProfiler() = 0;
    [[nodiscard]] virtual const Hardware::SubroutineProfiler& GetSubroutineProfiler() const = 0;
    virtual uint32_t* GetProfilerCounts() = 0;
    virtual uint32_t* GetProfilerReadCounts() = 0;
    virtual uint32_t* GetProfilerWriteCounts() = 0;

    virtual void SetHeatmapEnabled(bool enabled) = 0;
    virtual void ClearHeatmap() = 0;
    virtual void DecayHeatmap(uint8_t amount) = 0;

    virtual void SetAutoReload(bool enabled) = 0;
    [[nodiscard]] virtual bool IsAutoReloadEnabled() const = 0;

    [[nodiscard]] virtual Core::SavestateLoadResult GetLastLoadResult() const = 0;
    [[nodiscard]] virtual std::string GetLastLoadVersion() const = 0;
    [[nodiscard]] virtual std::string GetCurrentBinPath() const = 0;

    virtual void SetCartridge(const Core::Cartridge& cart) = 0;
    [[nodiscard]] virtual const Core::Cartridge& GetCartridge() const = 0;
    virtual Core::Cartridge& GetCartridge() = 0;
    virtual void ClearCartridge() = 0;

    virtual void SetACIAESPBridge(bool enabled) = 0;
    [[nodiscard]] virtual bool IsACIAESPBridge() const = 0;

    [[nodiscard]] virtual bool HasError() const noexcept = 0;
    [[nodiscard]] virtual const std::string& GetLastError() const noexcept = 0;
    virtual void SetError(std::string_view msg) = 0;
    virtual void ClearError() noexcept = 0;
};

}  // namespace Backend
