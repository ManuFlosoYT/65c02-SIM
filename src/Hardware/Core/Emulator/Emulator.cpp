#include "Hardware/Core/Emulator.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>


using namespace Hardware;

namespace Core {

Emulator::Emulator() : cpu(), scriptEngine(*this) {
    components = {&bus, &cpu, &ram, &rom, &via, &sid, &acia, &sdcard, &esp8266, &lcd, &gpu, &consoleSerializable};
    SetupHardware();
}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    if (bin.empty()) {
        // Lock removed
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
    // Lock removed

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
        rom.WriteDirect(0xFFFC, 0x00);
        rom.WriteDirect(0xFFFD, 0x80);
    }

    currentBinPath = name;
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
    // Lock removed
    EnsureWatchpointWriteHook();

    int res = 0;
    bool hooks = bus.HasActiveHooks() || breakpointManager.HasAnyBreakpointsFast();

    if (!gpuEnabled) {
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

void Emulator::SyncHardwareCycles(bool cpuStepped, bool isNewInstruction) {
    if (baudDelay > 0) {
        baudDelay--;
    }

    via.Clock();
    if (via.isIRQAsserted()) {
        pendingInterruptAny.store(true, std::memory_order_relaxed);
    }

    if (cpuStepped && !cpu.cycleAccurate && isNewInstruction) {
        int extraCycles = cpu.remainingCycles;
        for (int i = 0; i < extraCycles; ++i) {
            via.Clock();
            if (via.isIRQAsserted()) {
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
    if (hasInput.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty() && !acia.HasIRQ() && (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
            char chr = inputBuffer.front();
            inputBuffer.pop_front();

            acia.ReceiveData(chr);
            pendingInterruptAny.store(true, std::memory_order_relaxed);

            baudDelay = 2000;
        }
        if (inputBuffer.empty()) {
            hasInput.store(false, std::memory_order_relaxed);
        }
    }
}

template <bool Debug>
int Emulator::Step() {
    int res = 0;
    bool cpuStepped = false;
    bool isNewInstruction = true;

    if (cpu.cycleAccurate) {
        isNewInstruction = (cpu.remainingCycles == 0);
        RunCPUTick<Debug, true>(res, cpuStepped);
    } else {
        RunCPUTick<Debug, false>(res, cpuStepped);
    }
    SyncHardwareCycles(cpuStepped, isNewInstruction);
    HandleInterrupts<Debug>();

    if (cpu.waiting && cpu.remainingCycles == 0) {
        return 0;
    }

    HandleSerialInput<Debug>();

    return res;
}
template int Emulator::Step<true>();
template int Emulator::Step<false>();

void Emulator::InjectKey(char key) {
    if (key == '\n') {
        key = '\r';
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(key);
    hasInput.store(true, std::memory_order_relaxed);
}

void Emulator::SetOutputCallback(std::function<void(char)> callback) { acia.SetOutputCallback(std::move(callback)); }

void Emulator::TriggerIRQ() {
    this->pendingIRQ.store(true, std::memory_order_relaxed);
    this->pendingInterruptAny.store(true, std::memory_order_relaxed);
}
void Emulator::TriggerNMI() {
    this->pendingNMI.store(true, std::memory_order_relaxed);
    this->pendingInterruptAny.store(true, std::memory_order_relaxed);
}

void Emulator::Reset() {
    // Lock removed
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
        bus.RegisterDevice(dev.start, dev.end, &sid, true, true);
    } else if (dev.name == "GPU") {
        bus.RegisterDevice(dev.start, dev.end, &gpu, true, true);
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

    bool sdCustomMapped = false;
    for (const auto& dev : cartridge.busDevices) {
        RegisterCartridgeDevice(dev, sdCustomMapped);
    }

    // MMIO SD device default if not mapped manually
    if (!sdCustomMapped) {
        bus.RegisterDevice(0x5008, 0x500B, &sdcard, cartridge.config.sdEnabled.value_or(this->sdEnabled), true);
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
    bus.RegisterDevice(0x4800, 0x481F, &sid, true, true);
    bus.RegisterDevice(0x2000, 0x3FFF, &gpu, true, true);
    bus.RegisterVirtualDevice(&lcd, true);
}

void Emulator::ResetHardwareDevices() {
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
    halted = false;
    bus.ClearDevices();
    bus.Init();
    watchpointWriteHookInstalled = false;
    cpu.Reset();
    pendingIRQ.store(false, std::memory_order_relaxed);
    pendingNMI.store(false, std::memory_order_relaxed);
    pendingInterruptAny.store(false, std::memory_order_relaxed);
    totalCycles = 0;
    rewindBuffer.clear();
    lastSaveTime = std::chrono::steady_clock::now();

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
    bool is_lcd_enabled = false;
    for (const auto& dev : bus.GetRegisteredDevices()) {
        if (dev.device == &lcd && dev.enabled) {
            is_lcd_enabled = true;
            break;
        }
    }

    if (is_lcd_enabled) {
        lcd.Update(val);
    }
}

}  // namespace Core
