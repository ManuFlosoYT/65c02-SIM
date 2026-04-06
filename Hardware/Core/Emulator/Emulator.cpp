#include "Hardware/Core/Emulator.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>


using namespace Hardware;

namespace Core {

Emulator::Emulator() : cpu(), scriptEngine(*this) {
    components = {&bus, &cpu, &ram, &rom, &via, &sid, &acia, &sdcard, &esp8266, &lcd, &gpu, &consoleSerializable};
    SetupHardware();
}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    if (bin.empty()) {
        std::lock_guard<std::recursive_mutex> lock(emulationMutex);
        SetupHardware();
        currentBinPath = "";
        return true;
    }

    FILE* file = fopen(bin.c_str(), "rb");  // NOLINT
    if (file == nullptr) {
        errorMsg = "Error opening file " + bin;
        std::cerr << errorMsg << "\n";
        return false;
    }

    (void)fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize != ROM_SIZE) {
        errorMsg = "Error: File size mismatch (expected 32KB, got " + std::to_string(fileSize) + ")";
        (void)fclose(file);  // NOLINT
        std::cerr << errorMsg << "\n";
        return false;
    }
    (void)fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> buffer(ROM_SIZE);
    if (fread(buffer.data(), 1, ROM_SIZE, file) != ROM_SIZE) {
        errorMsg = "Error reading file " + bin;
        (void)fclose(file);  // NOLINT
        std::cerr << errorMsg << "\n";
        return false;
    }
    (void)fclose(file);  // NOLINT

    return InitFromMemory(buffer.data(), buffer.size(), bin, errorMsg);
}

bool Emulator::InitFromMemory(const uint8_t* data, size_t size, const std::string& name, std::string& errorMsg) {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    SetupHardware();

    if (size != ROM_SIZE && size != 0) {
        errorMsg = "Error: ROM data size mismatch (expected 32KB or 0, got " + std::to_string(size) + ")";
        std::cerr << errorMsg << "\n";
        return false;
    }

    if (size == ROM_SIZE) {
        for (size_t i = 0; i < ROM_SIZE; ++i) {
            rom.WriteDirect(static_cast<Word>(i), data[i]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    } else if (size == 0) {
        // VRAM only cartridge or empty ROM. Fill with HLT (0xDB) and set reset vector to self
        for (size_t i = 0; i < ROM_SIZE; ++i) {
            rom.WriteDirect(static_cast<Word>(i), 0xDB); 
        }
        rom.WriteDirect(0xFFFC, 0x00);
        rom.WriteDirect(0xFFFD, 0x80); // Reset to 0x8000 (standard ROM start)
    }

    currentBinPath = name;
    SetupHardware();
    std::cout << "Loaded ROM from memory: " << name << " (" << size << " bytes)\n";

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
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);

    int res = 0;
    bool hooks = bus.HasActiveHooks();

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

template <bool Debug>
int Emulator::Step() {
    int res = 0;
    bool cpuStepped = false;

    bool isNewInstruction = (cpu.remainingCycles == 0);
    if (gpuEnabled) {
        gpu.Clock();
        if (gpu.IsInBlankingInterval()) {
            res = cpu.Step<Debug>(bus);
            cpuStepped = true;
        }
    } else {
        res = cpu.Step<Debug>(bus);
        cpuStepped = true;
    }
    
    if (baudDelay > 0) {
        baudDelay--;
    }

    via.Clock();

    if (cpuStepped && !cpu.cycleAccurate && isNewInstruction) {
        int extraCycles = cpu.remainingCycles;
        for (int i = 0; i < extraCycles; ++i) {
            via.Clock();
        }
        cpu.remainingCycles = 0;
    }

    bool irq = acia.HasIRQ() || via.isIRQAsserted() || this->pendingIRQ.load(std::memory_order_relaxed);
    if (this->pendingNMI.load(std::memory_order_relaxed)) {
        this->pendingNMI.store(false, std::memory_order_relaxed);
        cpu.NMI<Debug>(bus);
        cpu.waiting = false;
    } else if (irq) {
        if (cpu.I == 0) {
            cpu.IRQ<Debug>(bus);
            this->pendingIRQ.store(false, std::memory_order_relaxed);
        }
        cpu.waiting = false;
    } else if (cpu.waiting) {
        return 0;
    }

    // Check input atomically. 
    // To ensure determinism for TAS/Scripts, we process input as soon as it's available 
    // instead of waiting for a 10,000 cycle threshold.
    if (hasInput.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty() && !acia.HasIRQ() && (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
            char chr = inputBuffer.front();
            inputBuffer.pop_front();

            acia.ReceiveData(chr);
            cpu.IRQ<Debug>(bus);

            baudDelay = 2000;
        }
        if (inputBuffer.empty()) {
            hasInput.store(false, std::memory_order_relaxed);
        }
    }
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

void Emulator::TriggerIRQ() { this->pendingIRQ.store(true); }
void Emulator::TriggerNMI() { this->pendingNMI.store(true); }

void Emulator::Reset() {
    std::lock_guard<std::recursive_mutex> lock(emulationMutex);
    SetupHardware();
}

void Emulator::SetupHardware() {
    halted = false;
    bus.ClearDevices();
    bus.Init();
    cpu.Reset();
    totalCycles = 0;
    rewindBuffer.clear();
    lastSaveTime = std::chrono::steady_clock::now();

    if (cartridge.loaded && !cartridge.busDevices.empty()) {
        for (const auto& dev : cartridge.busDevices) {
            if (dev.name == "RAM") {
                bus.RegisterDevice(dev.start, dev.end, &ram, true, false);
            } else if (dev.name == "ROM") {
                bus.RegisterDevice(dev.start, dev.end, &rom, true, false);
            } else if (dev.name == "ACIA") {
                bus.RegisterDevice(dev.start, dev.end, &acia, true, true);
            } else if (dev.name == "VIA") {
                bus.RegisterDevice(dev.start, dev.end, &via, true, true);
            } else if (dev.name == "ESP8266") {
                bus.RegisterDevice(dev.start, dev.end, &esp8266, cartridge.config.espEnabled.value_or(this->espEnabled), true);
            } else if (dev.name == "SID") {
                bus.RegisterDevice(dev.start, dev.end, &sid, true, true);
            } else if (dev.name == "GPU") {
                bus.RegisterDevice(dev.start, dev.end, &gpu, true, true);
            }
        }
        // MMIO SD device
        bus.RegisterDevice(0x5008, 0x500B, &sdcard, cartridge.config.sdEnabled.value_or(this->sdEnabled), true);
    } else {
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

    acia.Reset();
    via.Reset();
    sid.Reset();
    gpu.Reset();
    lcd.Reset();
    sdcard.Reset();
    esp8266.Reset();

    // Load VRAM from cartridge if present
    if (cartridge.loaded && !cartridge.vramData.empty()) {
        gpu.LoadVRAM(cartridge.vramData);
    }

    // Load SD image if present
    if (!currentBinPath.empty()) {
        std::filesystem::path sdPath = currentBinPath;
        sdPath.replace_extension(".sd");
        if (std::filesystem::exists(sdPath)) {
            sdcard.Mount(sdPath.string());
        }
    }

    via.SetPortBCallback([this](Byte val) { HandleVIAPortB(val); });

    if (breakpointManager.HasWatchpoints()) {
        bus.AddGlobalWriteHook([this](Word addr, Byte data) { breakpointManager.NotifyWrite(addr, data); });
    }
}

void Emulator::HandleVIAPortB(Byte val) {
    bool is_lcd_enabled = false;
    bool is_sd_enabled = false;
    for (const auto& dev : bus.GetRegisteredDevices()) {
        if (dev.device == &lcd && dev.enabled) {
            is_lcd_enabled = true;
        } else if (dev.device == static_cast<IBusDevice*>(&sdcard) && dev.enabled) {
            is_sd_enabled = true;
        }
    }

    if (is_lcd_enabled) {
        lcd.Update(val);
    }
}

}  // namespace Core
