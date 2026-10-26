#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/SubroutineProfiler.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>


using namespace Hardware;

namespace Core {

Emulator::Emulator() : cpu(), scriptEngine(*this), engine(*this), savestateManager(*this) {
    components = {&bus, &cpu, &ram, &rom, &via, &sid, &acia, &sdcard, &esp8266, &lcd, &gpu, &consoleSerializable};
    esp8266.SetRxByteCallback([this](Byte b) {
        if (serialBridge.IsACIAESPBridge()) {
            esp8266.RecordTraffic(TrafficDirection::RX, TrafficChannel::ATResponse, -1, "ESP8266 -> ACIA",
                                  std::string(1, static_cast<char>(b)));
            acia.ReceiveData(b);
        }
    });
    cpu.SetProfiler(&subroutineProfiler);
    SetupHardware();
}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    if (bin.empty()) {
        SetupHardware();
        currentBinPath = "";
        return true;
    }

    std::ifstream file(bin, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "Error opening file " + bin;
        std::cerr << errorMsg << "\n";
        return false;
    }

    std::uintmax_t fileSize = std::filesystem::file_size(bin);
    if (fileSize > ROM_SIZE) {
        errorMsg = "Error: File size too large (max 32KB, got " + std::to_string(fileSize) + ")";
        std::cerr << errorMsg << "\n";
        return false;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (buffer.size() != fileSize) {
        errorMsg = "Error reading file " + bin;
        std::cerr << errorMsg << "\n";
        return false;
    }

    return InitFromMemory(buffer, bin, errorMsg);
}

bool Emulator::InitFromMemory(std::span<const uint8_t> data, const std::string& name, std::string& errorMsg) {
    if (data.size() > ROM_SIZE) {
        errorMsg = "Error: ROM data too large (max 32KB, got " + std::to_string(data.size()) + ")";
        std::cerr << errorMsg << "\n";
        return false;
    }

    Byte* romRaw = rom.GetRawMemory();
    if (romRaw == nullptr || rom.GetRawMemorySize() < ROM_SIZE) {
        errorMsg = "Error: ROM raw memory is not available or too small";
        std::cerr << errorMsg << "\n";
        return false;
    }

    std::memset(romRaw, 0xDB, ROM_SIZE);
    if (!data.empty()) {
        std::memcpy(romRaw, data.data(), data.size());
    }

    if (data.empty()) {
        // VRAM only cartridge or empty ROM. Set reset vector to standard ROM start.
        rom.WriteDirect(0x7FFC, 0x00);
        rom.WriteDirect(0x7FFD, 0x80);
    }

    currentBinPath = name;
    
    serialBridge.Clear();
    
    SetupHardware();
    std::cout << "Loaded ROM from memory: " << name << " (" << data.size() << " bytes)\n";

    try {
#ifndef TARGET_WASM
        if (std::filesystem::exists(name)) {
            engine.SetLastBinModificationTime(std::filesystem::last_write_time(name));
        }
#endif
    } catch (const std::exception& e) {
        // Ignore filesystem errors as they are not critical for ROM loading
        (void)e;
    }

    return true;
}

int Emulator::Step() {
    bool recordSnapshot = savestateManager.IsRewindSnapshotsEnabled() && (engine.IsPaused() || !engine.IsRunning());
    if (recordSnapshot) {
        savestateManager.CapturePreStepSnapshot();
    }

    EnsureWatchpointWriteHook();

    int res = 0;
    bool hooks = bus.HasActiveHooks() || breakpointManager.HasAnyBreakpointsFast() ||
                 logicAnalyzerEnabled.load(std::memory_order_relaxed);

    if (!gpuEnabled || rasterBeamSlowMotion.load(std::memory_order_relaxed)) {
        res = hooks ? Step<true>() : Step<false>();
    } else {
        // Advance through the full drawing interval until the CPU executes one instruction
        static constexpr int MAX_GPU_STEP_CYCLES = GPU::DISPLAY_WIDTH * GPU::DISPLAY_HEIGHT;
        bool cpuRan = false;
        for (int i = 0; i < MAX_GPU_STEP_CYCLES && !cpuRan; ++i) {
            cpuRan = gpu.IsInBlankingInterval();
            res = hooks ? Step<true>() : Step<false>();
            if (res != 0) {
                break;
            }
        }
    }

    if (res != 0) {
        engine.HandleStop(res);
    }

    if (recordSnapshot) {
        savestateManager.SaveStateToBuffer();
    }
    return res;
}

template <bool Debug, bool CycleAccurateMode>
void Emulator::RunCPUTick(int& res, bool& cpuStepped) {
    if (gpuEnabled) {
        gpu.Clock();
        if (gpu.IsInBlankingInterval()) {
            if constexpr (CycleAccurateMode) {
                res = cpu.StepCycleAccurate<Debug>(bus);
            } else {
                res = cpu.StepFast<Debug>(bus);
            }
            cpuStepped = true;
        }
    } else {
        if constexpr (CycleAccurateMode) {
            res = cpu.StepCycleAccurate<Debug>(bus);
        } else {
            res = cpu.StepFast<Debug>(bus);
        }
        cpuStepped = true;
    }
}

void Emulator::SyncHardwareCycles(bool cpuStepped, bool isNewInstruction, int ips) {
    lastStepCpuCycles = cpuStepped ? 1 : 0;
    lastStepGpuCycles = (gpuEnabled && !cpuStepped) ? 1 : 0;
    serialBridge.DecrementBaudDelay();

    via.Clock();
    acia.Clock(ips);
    if (sidEnabled) {
        sid.Clock();
    }
    if (via.isIRQAsserted() || acia.HasIRQ()) [[unlikely]] {
        pendingInterruptAny.store(true, std::memory_order_relaxed);
    }

    if (cpuStepped && !cpu.cycleAccurate && isNewInstruction) {
        int extraCycles = cpu.remainingCycles;
        if (extraCycles > 0) {
            const auto extra = static_cast<uint32_t>(extraCycles);
            lastStepCpuCycles += extra;
            via.Clock(extra);
            if (!acia.IsIdle()) [[unlikely]] {
                for (int i = 0; i < extraCycles; ++i) {
                    acia.Clock(ips);
                }
            }
            if (sidEnabled) {
                sid.Clock(extra);
            }
            if (via.isIRQAsserted() || acia.HasIRQ()) [[unlikely]] {
                pendingInterruptAny.store(true, std::memory_order_relaxed);
            }
        }
        cpu.remainingCycles = 0;
    }
}

template <bool Debug>
void Emulator::HandleInterrupts() {
    if (!pendingInterruptAny.load(std::memory_order_relaxed)) {
        return;
    }

    if (!cpu.HasPendingNMI()) {
        this->pendingNMI.store(false, std::memory_order_relaxed);
    }
    bool nmiPending = this->pendingNMI.load(std::memory_order_relaxed);
    bool irqPending = this->pendingIRQ.load(std::memory_order_relaxed) || acia.HasIRQ() || via.isIRQAsserted();

    if (!nmiPending && !irqPending) {
        pendingInterruptAny.store(false, std::memory_order_relaxed);
        return;
    }

    if (nmiPending) {
        cpu.waiting = false;
        if (cpu.remainingCycles == 0) {
            this->pendingNMI.store(false, std::memory_order_relaxed);
            cpu.NMI<Debug>(bus);
        }
    } else if (irqPending) {
        bool wasWaiting = cpu.waiting;
        cpu.waiting = false;
        if (cpu.remainingCycles == 0) {
            if (cpu.I == 0) {
                cpu.IRQ<Debug>(bus);
                this->pendingIRQ.store(false, std::memory_order_relaxed);
            } else if (wasWaiting) {
                this->pendingIRQ.store(false, std::memory_order_relaxed);
            } else if (!acia.HasIRQ() && !via.isIRQAsserted()) {
                this->pendingIRQ.store(false, std::memory_order_relaxed);
            }
        }
    }

    bool stillPending = this->pendingNMI.load(std::memory_order_relaxed) || this->pendingIRQ.load(std::memory_order_relaxed) ||
                        acia.HasIRQ() || via.isIRQAsserted();
    pendingInterruptAny.store(stillPending, std::memory_order_relaxed);
}

template <bool Debug>
void Emulator::HandleSerialInput() {
    serialBridge.ProcessSerialInput(acia, engine.GetTargetIPS(), pendingInterruptAny);
}

template <bool Debug, bool CycleAccurateMode>
int Emulator::StepInternal(int ips) {
    int res = 0;
    bool cpuStepped = false;
    bool isNewInstruction = true;
    if constexpr (Debug) {
        bus.SetLastAccess(cpu.PC, bus.ReadDirect(cpu.PC), true);
    }

    if constexpr (CycleAccurateMode) {
        isNewInstruction = (cpu.remainingCycles == 0);
        RunCPUTick<Debug, true>(res, cpuStepped);
    } else {
        RunCPUTick<Debug, false>(res, cpuStepped);
    }
    SyncHardwareCycles(cpuStepped, isNewInstruction, ips);
    if (cpuStepped && pendingInterruptAny.load(std::memory_order_relaxed)) [[unlikely]] {
        HandleInterrupts<Debug>();
    }

    if (cpu.waiting && cpu.remainingCycles == 0) [[unlikely]] {
        return 0;
    }

    if (serialBridge.HasInput()) [[unlikely]] {
        HandleSerialInput<Debug>();
    }

    if constexpr (Debug) {
        if (logicAnalyzerEnabled.load(std::memory_order_relaxed)) [[unlikely]] {
            SampleLogicAnalyzer(bus.GetLastAddress(), bus.GetLastData(), bus.GetLastRw(), isNewInstruction);
        }
    }

    return res;
}

template <bool Debug>
int Emulator::Step() {
    int ips = engine.GetTargetIPS();
    if (ips <= 0) {
        ips = 1000000;
    }
    if (sidEnabled) {
        sid.SetCpuClockRate(static_cast<double>(ips));
    }
    bool isNewInstruction = !cpu.cycleAccurate || (cpu.remainingCycles == 0);
    int res = cpu.cycleAccurate ? StepInternal<Debug, true>(ips) : StepInternal<Debug, false>(ips);
    totalCycles.fetch_add(lastStepCpuCycles + lastStepGpuCycles, std::memory_order_relaxed);
    if (lastStepCpuCycles > 0 && isNewInstruction) {
        totalInstructions.fetch_add(1, std::memory_order_relaxed);
    }
    return res;
}
template int Emulator::Step<true>();
template int Emulator::Step<false>();
template int Emulator::StepInternal<true, true>(int);
template int Emulator::StepInternal<false, true>(int);
template int Emulator::StepInternal<true, false>(int);
template int Emulator::StepInternal<false, false>(int);
template void Emulator::HandleInterrupts<true>();
template void Emulator::HandleInterrupts<false>();
template void Emulator::HandleSerialInput<true>();
template void Emulator::HandleSerialInput<false>();

void Emulator::InjectKey(char key) {
    serialBridge.InjectKey(key);
}

void Emulator::SetOutputCallback(std::function<void(char)> callback) {
    acia.SetOutputCallback([this, cb = std::move(callback)](char c) {
        serialBridge.ForwardOutputIfBridged(c, esp8266);
        if (cb) {
            cb(c);
        }
    });
}

void Emulator::TriggerIRQ() {
    this->pendingIRQ.store(true, std::memory_order_relaxed);
    this->pendingInterruptAny.store(true, std::memory_order_relaxed);
}
void Emulator::TriggerNMI() {
    this->pendingNMI.store(true, std::memory_order_relaxed);
    this->pendingInterruptAny.store(true, std::memory_order_relaxed);
    cpu.TriggerNMI();
}

void Emulator::Reset() {
    SetupHardware();
}

void Emulator::SetupHardware() {
    SystemConfigurator::SetupHardware(*this);
}

void Emulator::SetSIDEnabled(bool enabled) {
    if (sidEnabledOverride && sidEnabled == enabled) {
        return;
    }
    sidEnabledOverride = true;
    sidEnabled = enabled;
    SetupHardware();
}

void Emulator::EnsureWatchpointWriteHook() {
    bool hasWatchpoints = breakpointManager.HasWatchpointsFast();
    if (hasWatchpoints && !watchpointWriteHookInstalled) {
        bus.AddGlobalWriteHook([this](Word addr, Byte data) { breakpointManager.NotifyWrite(addr, data); });
        watchpointWriteHookInstalled = true;
        return;
    }

    if (!hasWatchpoints && watchpointWriteHookInstalled) {
        bus.ClearGlobalWriteHooks();
        watchpointWriteHookInstalled = false;
        return;
    }
}

void Emulator::HandleVIAPortB(Byte val) {
    const auto& devices = bus.GetRegisteredDevices();
    for (const auto& dev : std::views::reverse(devices)) {
        if (dev.device == &lcd) {
            if (dev.enabled) {
                lcd.Update(val);
            }
            return;
        }
    }
}

void Emulator::SampleLogicAnalyzer(Word addr, Byte data, bool rw, bool sync) {
    if (!logicAnalyzerEnabled.load(std::memory_order_relaxed)) {
        return;
    }
    LogicSample sample;
    sample.cycle = totalCycles.load(std::memory_order_relaxed);
    sample.address = addr;
    sample.data = data;
    sample.rw = rw ? 1 : 0;
    sample.sync = sync ? 1 : 0;
    sample.rdy = 1;
    sample.irq = (via.isIRQAsserted() || acia.HasIRQ() || pendingIRQ.load(std::memory_order_relaxed)) ? 1 : 0;
    sample.nmi = pendingNMI.load(std::memory_order_relaxed) ? 1 : 0;
    sample.ca1 = via.GetCA1() ? 1 : 0;
    sample.ca2 = via.GetCA2() ? 1 : 0;
    sample.cb1 = via.GetCB1() ? 1 : 0;
    sample.cb2 = via.GetCB2() ? 1 : 0;
    sample.spiCs = !sdcard.IsCSActive();
    sample.spiClk = sdcard.IsCSActive() ? 1 : 0;
    sample.spiMosi = (sdcard.GetLastMOSI() & 0x80) ? 1 : 0;
    sample.spiMiso = (sdcard.GetLastMISO() & 0x80) ? 1 : 0;
    sample.uartTx = acia.GetTxPin() ? 1 : 0;
    sample.uartRx = acia.GetRxPin() ? 1 : 0;
    sample.portA = via.GetPortA();
    sample.portB = via.GetPortB();
    logicAnalyzer.RecordSample(sample);
}

int Emulator::ProcessBatch(int count, bool hooks, bool hasBreakpoints, bool hasComplex, int& batchInstructions,
                           uint64_t& batchCpuCycles, uint64_t& batchGpuCycles, int& stopCode) {
    stopCode = std::numeric_limits<int>::min();
    int ips = engine.GetTargetIPS();
    if (ips <= 0) {
        ips = 1000000;
    }
    if (sidEnabled) {
        sid.SetCpuClockRate(static_cast<double>(ips));
    }

    if (cpu.cycleAccurate) {
        if (!hooks && !hasBreakpoints) [[likely]] {
            uint64_t localCpuCycles = 0;
            uint64_t localGpuCycles = 0;
            int localInstructions = 0;
            int i = 0;
            while (i < count) {
                if (gpuEnabled && !gpu.IsInBlankingInterval()) {
                    const uint32_t waitCycles = gpu.GetDrawingWaitCycles();
                    if (waitCycles > 0 && (!sidEnabled || sid.IsDormant()) && via.IsIdle() &&
                        acia.IsIdle() && serialBridge.GetBaudDelay() == 0 &&
                        !pendingInterruptAny.load(std::memory_order_relaxed) &&
                        !serialBridge.HasInput()) [[likely]] {
                        const auto rem = static_cast<uint32_t>(count - i);
                        const uint32_t skip = (rem < waitCycles) ? rem : waitCycles;
                        gpu.SkipDrawingCycles(skip);
                        via.Clock(skip);
                        if (sidEnabled) {
                            sid.Clock(skip);
                        }
                        localGpuCycles += skip;
                        i += static_cast<int>(skip);
                        continue;
                    }
                }

                const bool isNew = (cpu.remainingCycles == 0);
                bool cpuStepped = false;
                int res = 0;
                if (gpuEnabled) {
                    gpu.Clock();
                    if (gpu.IsInBlankingInterval()) {
                        res = cpu.StepCycleAccurate<false>(bus);
                        cpuStepped = true;
                    }
                } else {
                    res = cpu.StepCycleAccurate<false>(bus);
                    cpuStepped = true;
                }

                if (cpuStepped) {
                    localCpuCycles++;
                    if (isNew) {
                        localInstructions++;
                    }
                } else {
                    localGpuCycles++;
                }

                serialBridge.DecrementBaudDelay();
                via.Clock();
                acia.Clock(ips);
                if (sidEnabled) {
                    sid.Clock();
                }
                if (via.isIRQAsserted() || acia.HasIRQ()) [[unlikely]] {
                    pendingInterruptAny.store(true, std::memory_order_relaxed);
                }
                if (cpuStepped && pendingInterruptAny.load(std::memory_order_relaxed)) [[unlikely]] {
                    HandleInterrupts<false>();
                }
                if (serialBridge.HasInput()) [[unlikely]] {
                    HandleSerialInput<false>();
                }

                ++i;
                if (res != 0) [[unlikely]] {
                    batchInstructions += localInstructions;
                    batchCpuCycles += localCpuCycles;
                    batchGpuCycles += localGpuCycles;
                    lastStepCpuCycles = cpuStepped ? 1 : 0;
                    lastStepGpuCycles = (gpuEnabled && !cpuStepped) ? 1 : 0;
                    stopCode = res;
                    return i;
                }
            }
            batchInstructions += localInstructions;
            batchCpuCycles += localCpuCycles;
            batchGpuCycles += localGpuCycles;
            return count;
        }
        for (int i = 0; i < count; ++i) {
            bool isNew = (cpu.remainingCycles == 0);
            if (hasBreakpoints && isNew) {
                if (!cpu.isInit) {
                    cpu.EnsureInit(bus);
                }
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        stopCode = 0;
                        return i;
                    }
                }
            }
            int res = hooks ? StepInternal<true, true>(ips) : StepInternal<false, true>(ips);

            if (isNew && lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += lastStepCpuCycles;
            batchGpuCycles += lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }
        }
    } else {
        if (!hooks && !hasBreakpoints) [[likely]] {
            for (int i = 0; i < count; ++i) {
                int res = StepInternal<false, false>(ips);
                if (lastStepCpuCycles > 0) {
                    batchInstructions++;
                }
                batchCpuCycles += lastStepCpuCycles;
                batchGpuCycles += lastStepGpuCycles;
                if (res != 0) [[unlikely]] {
                    stopCode = res;
                    return i + 1;
                }
            }
            return count;
        }
        for (int i = 0; i < count; ++i) {
            if (hasBreakpoints) {
                if (!cpu.isInit) {
                    cpu.EnsureInit(bus);
                }
                if (hasComplex || breakpointManager.IsPCBreakpoint(cpu.PC)) [[unlikely]] {
                    if (breakpointManager.Evaluate(cpu, bus) > 0) {
                        stopCode = 0;
                        return i;
                    }
                }
            }
            int res = hooks ? StepInternal<true, false>(ips) : StepInternal<false, false>(ips);

            if (lastStepCpuCycles > 0) {
                batchInstructions++;
            }
            batchCpuCycles += lastStepCpuCycles;
            batchGpuCycles += lastStepGpuCycles;

            if (res != 0) [[unlikely]] {
                stopCode = res;
                return i + 1;
            }
        }
    }
    return count;
}

void Emulator::SaveInternalState(std::ostream& stateStream) {
    int baud = serialBridge.GetBaudDelay();
    Hardware::ISerializable::Serialize(stateStream, baud);

    bool gpuE = gpuEnabled;
    Hardware::ISerializable::Serialize(stateStream, gpuE);

    int tIPS = GetTargetIPS();
    Hardware::ISerializable::Serialize(stateStream, tIPS);

    serialBridge.SerializeBuffer(stateStream);

    auto binPathLen = static_cast<uint32_t>(currentBinPath.length());
    Hardware::ISerializable::Serialize(stateStream, binPathLen);
    stateStream.write(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = IsAutoReloadEnabled();
    Hardware::ISerializable::Serialize(stateStream, autoReload);

    uint64_t totalCyc = totalCycles.load(std::memory_order_relaxed);
    Hardware::ISerializable::Serialize(stateStream, totalCyc);

    uint64_t totalInst = totalInstructions.load(std::memory_order_relaxed);
    Hardware::ISerializable::Serialize(stateStream, totalInst);

    Hardware::ISerializable::Serialize(stateStream, espEnabled);
    Hardware::ISerializable::Serialize(stateStream, sdEnabled);
    bool aciaEsp = serialBridge.IsACIAESPBridge();
    Hardware::ISerializable::Serialize(stateStream, aciaEsp);
}

bool Emulator::LoadInternalState(std::istream& stateStream) {
    int baud = 0;
    Hardware::ISerializable::Deserialize(stateStream, baud);
    serialBridge.SetBaudDelay(baud);

    bool gpuE = false;
    Hardware::ISerializable::Deserialize(stateStream, gpuE);
    gpuEnabled = gpuE;

    int tIPS = 0;
    Hardware::ISerializable::Deserialize(stateStream, tIPS);
    SetTargetIPS(tIPS);

    if (!serialBridge.DeserializeBuffer(stateStream)) {
        return false;
    }

    uint32_t binPathLen = 0;
    Hardware::ISerializable::Deserialize(stateStream, binPathLen);
    if (binPathLen > 4096) { return false; }
    currentBinPath.assign(binPathLen, '\0');
    stateStream.read(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = false;
    Hardware::ISerializable::Deserialize(stateStream, autoReload);
    SetAutoReload(autoReload);

    uint64_t totalCyc = 0;
    Hardware::ISerializable::Deserialize(stateStream, totalCyc);
    totalCycles.store(totalCyc, std::memory_order_relaxed);

    uint64_t totalInst = 0;
    Hardware::ISerializable::Deserialize(stateStream, totalInst);
    totalInstructions.store(totalInst, std::memory_order_relaxed);

    Hardware::ISerializable::Deserialize(stateStream, espEnabled);
    Hardware::ISerializable::Deserialize(stateStream, sdEnabled);
    bool aciaEsp = false;
    Hardware::ISerializable::Deserialize(stateStream, aciaEsp);
    serialBridge.SetACIAESPBridge(aciaEsp);

    return true;
}

}  // namespace Core
