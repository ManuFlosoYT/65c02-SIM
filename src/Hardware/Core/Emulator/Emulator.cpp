#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/SubroutineProfiler.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>


using namespace Hardware;

namespace Core {

Emulator::Emulator() : cpu(), scriptEngine(*this) {
    components = {&bus, &cpu, &ram, &rom, &via, &sid, &acia, &sdcard, &esp8266, &lcd, &gpu, &consoleSerializable};
    esp8266.SetRxByteCallback([this](Byte b) {
        if (aciaEspBridge) {
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
    
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        inputBuffer.clear();
        hasInput.store(false, std::memory_order_relaxed);
    }
    
    SetupHardware();
    std::cout << "Loaded ROM from memory: " << name << " (" << data.size() << " bytes)\n";

    try {
#ifndef TARGET_WASM
        if (std::filesystem::exists(name)) {
            lastBinModificationTime = std::filesystem::last_write_time(name);
        }
#endif
    } catch (const std::exception& e) {
        // Ignore filesystem errors as they are not critical for ROM loading
        (void)e;
    }

    return true;
}

int Emulator::Step() {
    bool recordSnapshot = rewindSnapshotsEnabled && (paused.load() || !running.load());
    if (recordSnapshot) {
        std::scoped_lock lock(rewindMutex);
        std::stringstream ss;
        if (SaveComponentsState(ss)) {
            SaveInternalState(ss);
            lastCapturedState = ss.str();
            lastCapturedCycles = totalCycles.load(std::memory_order_relaxed);
            lastCapturedInstructions = totalInstructions.load(std::memory_order_relaxed);
        }
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
        Pause();
#ifndef TARGET_WASM
        if (totalCycles > totalCyclesAtLastResume) {
            sid.StopRecording();
        }
#endif
        halted = true;
    }

    if (recordSnapshot) {
        SaveStateToBuffer();
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
    if (baudDelay > 0) [[unlikely]] {
        baudDelay--;
    }

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
        cpu.waiting = false;
        if (cpu.remainingCycles == 0) {
            if (cpu.I == 0) {
                cpu.IRQ<Debug>(bus);
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
    if (!hasInput.load(std::memory_order_relaxed)) {
        return;
    }

    if (baudDelay > 0 || !acia.CanReceive() || (via.GetPortA() & 0x01) != 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    if (!inputBuffer.empty() && acia.CanReceive() && (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
        char chr = inputBuffer.front();
        inputBuffer.pop_front();

        acia.ReceiveData(chr);
        if (acia.HasIRQ()) {
            pendingInterruptAny.store(true, std::memory_order_relaxed);
        }
        
        int baudRate = acia.GetBaudRate();
        if (baudRate <= 0) baudRate = 19200;
        int ips = targetIPS.load(std::memory_order_relaxed);
        if (ips <= 0) ips = 1000000;
        baudDelay = (ips * 10) / baudRate;
    }

    if (inputBuffer.empty()) {
        hasInput.store(false, std::memory_order_relaxed);
    }
}

template <bool Debug, bool CycleAccurateMode>
int Emulator::StepInternal(int ips) {
    int res = 0;
    bool cpuStepped = false;
    bool isNewInstruction = true;
    Word sampleAddr = cpu.PC;

    if constexpr (CycleAccurateMode) {
        isNewInstruction = (cpu.remainingCycles == 0);
        RunCPUTick<Debug, true>(res, cpuStepped);
    } else {
        RunCPUTick<Debug, false>(res, cpuStepped);
    }
    SyncHardwareCycles(cpuStepped, isNewInstruction, ips);
    if (pendingInterruptAny.load(std::memory_order_relaxed)) [[unlikely]] {
        HandleInterrupts<Debug>();
    }

    if (cpu.waiting && cpu.remainingCycles == 0) [[unlikely]] {
        return 0;
    }

    if (hasInput.load(std::memory_order_relaxed)) [[unlikely]] {
        HandleSerialInput<Debug>();
    }

    if constexpr (Debug) {
        if (logicAnalyzerEnabled.load(std::memory_order_relaxed)) [[unlikely]] {
            SampleLogicAnalyzer(sampleAddr, bus.ReadDirect(sampleAddr), true, isNewInstruction);
        }
    }

    return res;
}

template <bool Debug>
int Emulator::Step() {
    int ips = targetIPS.load(std::memory_order_relaxed);
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
    if (key == '\n') {
        key = '\r';
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(key);
    hasInput.store(true, std::memory_order_relaxed);
}

void Emulator::SetOutputCallback(std::function<void(char)> callback) {
    acia.SetOutputCallback([this, cb = std::move(callback)](char c) {
        if (aciaEspBridge) {
            esp8266.RecordTraffic(TrafficDirection::TX, TrafficChannel::ATCommand, -1, "ACIA -> ESP8266",
                                  std::string(1, c));
            esp8266.Write(0x5004, static_cast<Byte>(c));
        }
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
}

void Emulator::Reset() {
    SetupHardware();
}

void Emulator::RegisterCartridgeDevice(const DeviceConfig& dev, bool& sdCustomMapped) {
    if (dev.name == "RAM") {
        bus.RegisterDevice(dev.start, dev.end, &ram, true, false);
    } else if (dev.name == "ROM") {
        bus.RegisterDevice(dev.start, dev.end, &rom, true, false);
    } else if (dev.name == "ACIA") {
        bus.RegisterDevice(dev.start, dev.end, &acia, true, true);
    } else if (dev.name == "VIA") {
        bus.RegisterDevice(dev.start, dev.end, &via, true, true);
    } else if (dev.name == "ESP8266" || dev.name == "ESP32") {
        bus.RegisterDevice(dev.start, dev.end, &esp8266, cartridge.config.espEnabled.value_or(this->espEnabled), true);
    } else if (dev.name == "SID") {
        sidEnabled = sidEnabledOverride
                         ? sidEnabled
                         : cartridge.config.sidEnabled.value_or(true);
        bus.RegisterDevice(dev.start, dev.end, &sid, sidEnabled, true);
    } else if (dev.name == "GPU") {
        bus.RegisterDevice(dev.start, dev.end, &gpu, cartridge.config.gpuEnabled.value_or(this->gpuEnabled), true);
    } else if (dev.name == "LCD") {
        bus.RegisterVirtualDevice(&lcd, true);
    } else if (dev.name == "SD Card") {
        bus.RegisterDevice(dev.start, dev.end, &sdcard, cartridge.config.sdEnabled.value_or(this->sdEnabled), true);
        sdCustomMapped = true;
    } else {
        throw std::runtime_error("Unsupported cartridge device: " + dev.name);
    }
}

bool Emulator::RegisterCartridgeLayout() {
    if (!cartridge.loaded || cartridge.busDevices.empty()) {
        return false;
    }

    if (!sidEnabledOverride) {
        sidEnabled = false;
    }
    bool sdCustomMapped = false;
    bool lcdCustomMapped = false;
    for (const auto& dev : cartridge.busDevices) {
        if (dev.name == "LCD") {
            lcdCustomMapped = true;
        }
        RegisterCartridgeDevice(dev, sdCustomMapped);
    }

    // MMIO SD device default if not mapped manually
    if (!sdCustomMapped) {
        bus.RegisterDevice(0x5008, 0x500B, &sdcard, cartridge.config.sdEnabled.value_or(this->sdEnabled), true);
    }

    if (!lcdCustomMapped) {
        bus.RegisterVirtualDevice(&lcd, true);
    }

    return true;
}

void Emulator::RegisterDefaultLayout() {
    // Default layout (v1.0)
    bus.RegisterDevice(0x0000, 0x7FFF, &ram, true, false);
    bus.RegisterDevice(0x8000, 0xFFFF, &rom, true, false);
    bus.RegisterDevice(0x5000, 0x5003, &acia, true, true);
    bus.RegisterDevice(0x6000, 0x600F, &via, true, true);
    bus.RegisterDevice(0x5004, 0x5007, &esp8266, cartridge.config.espEnabled.value_or(this->espEnabled), true);
    bus.RegisterDevice(0x5008, 0x500B, &sdcard, cartridge.config.sdEnabled.value_or(this->sdEnabled), true);
    if (!sidEnabledOverride) {
        sidEnabled = cartridge.config.sidEnabled.value_or(true);
    }
    bus.RegisterDevice(0x4800, 0x481F, &sid, sidEnabled, true);
    bus.RegisterDevice(0x2000, 0x3FFF, &gpu, cartridge.config.gpuEnabled.value_or(this->gpuEnabled), true);
    bus.RegisterVirtualDevice(&lcd, true);
}

void Emulator::ResetHardwareDevices() {
    if (ram.IsPowerOnPattern()) {
        ram.Reset();
    }
    acia.Reset();
    via.Reset();
    sid.Reset();
    gpu.Reset();
    lcd.Reset();
    sdcard.Reset();
    esp8266.Reset();
}

void Emulator::LoadCartridgeVRAMIfPresent() {
    if (cartridge.loaded && !cartridge.vramData.empty()) {
        gpu.LoadVRAM(cartridge.vramData);
    }
}

void Emulator::MountSDCardIfPresent() {
    // Priority: 1. Cartridge internal SD, 2. External .sd file alongside ROM
    if (cartridge.loaded && !cartridge.sdCardPath.empty()) {
        // Persistence: Re-mount if lost during reset (though SDCard::Reset doesn't unmount)
        if (!sdcard.IsMounted() || sdcard.GetMountedPath() != cartridge.sdCardPath) {
            sdcard.Mount(cartridge.sdCardPath);
        }
        return;
    }

    if (currentBinPath.empty()) {
        return;
    }

    std::filesystem::path sdPath = currentBinPath;
    sdPath.replace_extension(".sd");
    if (std::filesystem::exists(sdPath)) {
        if (!sdcard.IsMounted() || sdcard.GetMountedPath() != sdPath.string()) {
            sdcard.Mount(sdPath.string());
        }
    }
}

void Emulator::SetupHardware() {
    if (running && std::this_thread::get_id() != emulatorThread.get_id() && !isActuallyPaused.load()) {
        WaitUntilSafeToMutate();
    }
    halted = false;
    bus.ClearDevices();
    bus.Init();
    watchpointWriteHookInstalled = false;
    cpu.SetProfiler(&subroutineProfiler);
    cpu.Reset();
    pendingIRQ.store(false, std::memory_order_relaxed);
    pendingNMI.store(false, std::memory_order_relaxed);
    pendingInterruptAny.store(false, std::memory_order_relaxed);
    totalCycles = 0;
    totalInstructions = 0;
    ClearRewindBuffer();

    try {
        if (!RegisterCartridgeLayout()) {
            RegisterDefaultLayout();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during SetupHardware: " << e.what() << '\n';
        if (cartridge.loaded) {
            cartridge.loaded = false;
            SetupHardware(); // Retry with default layout
            return;
        }
    }

    ResetHardwareDevices();

    LoadCartridgeVRAMIfPresent();
    MountSDCardIfPresent();

    via.SetPortBCallback([this](Byte val) { HandleVIAPortB(val); });
    EnsureWatchpointWriteHook();
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
    sample.uartTx = acia.GetTxPin() ? 1 : 0;
    sample.uartRx = acia.GetRxPin() ? 1 : 0;
    sample.portA = via.GetPortA();
    sample.portB = via.GetPortB();
    logicAnalyzer.RecordSample(sample);
}

}  // namespace Core
