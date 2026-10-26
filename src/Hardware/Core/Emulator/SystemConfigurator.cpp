#include "Hardware/Core/Emulator/SystemConfigurator.h"

#include <filesystem>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <thread>

#include "Hardware/Core/Emulator/Emulator.h"

namespace Core {

void SystemConfigurator::RegisterCartridgeDevice(Emulator& emu, const DeviceConfig& dev, bool& sdCustomMapped) {
    if (dev.name == "RAM") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.ram, true, false);
    } else if (dev.name == "ROM") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.rom, true, false);
    } else if (dev.name == "ACIA") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.acia, true, true);
    } else if (dev.name == "VIA") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.via, true, true);
    } else if (dev.name == "SID") {
        if (!emu.sidEnabledOverride) {
            emu.sidEnabled = emu.cartridge.config.sidEnabled.value_or(true);
        }
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.sid, emu.sidEnabled, true);
    } else if (dev.name == "ESP8266") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.esp8266, emu.cartridge.config.espEnabled.value_or(emu.espEnabled), true);
    } else if (dev.name == "GPU") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.gpu, emu.cartridge.config.gpuEnabled.value_or(emu.gpuEnabled), true);
    } else if (dev.name == "LCD") {
        emu.bus.RegisterVirtualDevice(&emu.lcd, true);
    } else if (dev.name == "SD Card") {
        emu.bus.RegisterDevice(dev.start, dev.end, &emu.sdcard, emu.cartridge.config.sdEnabled.value_or(emu.sdEnabled), true);
        sdCustomMapped = true;
    } else {
        throw std::runtime_error("Unsupported cartridge device: " + dev.name);
    }
}

bool SystemConfigurator::RegisterCartridgeLayout(Emulator& emu) {
    if (!emu.cartridge.loaded || emu.cartridge.busDevices.empty()) {
        return false;
    }

    if (!emu.sidEnabledOverride) {
        emu.sidEnabled = false;
    }
    bool sdCustomMapped = false;
    bool lcdCustomMapped = false;
    for (const auto& dev : emu.cartridge.busDevices) {
        if (dev.name == "LCD") {
            lcdCustomMapped = true;
        }
        RegisterCartridgeDevice(emu, dev, sdCustomMapped);
    }

    if (!sdCustomMapped) {
        emu.bus.RegisterDevice(0x5008, 0x500B, &emu.sdcard, emu.cartridge.config.sdEnabled.value_or(emu.sdEnabled), true);
    }

    if (!lcdCustomMapped) {
        emu.bus.RegisterVirtualDevice(&emu.lcd, true);
    }

    return true;
}

void SystemConfigurator::RegisterDefaultLayout(Emulator& emu) {
    emu.bus.RegisterDevice(0x0000, 0x7FFF, &emu.ram, true, false);
    emu.bus.RegisterDevice(0x8000, 0xFFFF, &emu.rom, true, false);
    emu.bus.RegisterDevice(0x5000, 0x5003, &emu.acia, true, true);
    emu.bus.RegisterDevice(0x6000, 0x600F, &emu.via, true, true);
    emu.bus.RegisterDevice(0x5004, 0x5007, &emu.esp8266, emu.cartridge.config.espEnabled.value_or(emu.espEnabled), true);
    emu.bus.RegisterDevice(0x5008, 0x500B, &emu.sdcard, emu.cartridge.config.sdEnabled.value_or(emu.sdEnabled), true);
    if (!emu.sidEnabledOverride) {
        emu.sidEnabled = emu.cartridge.config.sidEnabled.value_or(true);
    }
    emu.bus.RegisterDevice(0x4800, 0x481F, &emu.sid, emu.sidEnabled, true);
    emu.bus.RegisterDevice(0x2000, 0x3FFF, &emu.gpu, emu.cartridge.config.gpuEnabled.value_or(emu.gpuEnabled), true);
    emu.bus.RegisterVirtualDevice(&emu.lcd, true);
}

void SystemConfigurator::ResetHardwareDevices(Emulator& emu) {
    if (emu.ram.IsPowerOnPattern()) {
        emu.ram.Reset();
    }
    emu.acia.Reset();
    emu.via.Reset();
    emu.sid.Reset();
    emu.gpu.Reset();
    emu.lcd.Reset();
    emu.sdcard.Reset();
    emu.esp8266.Reset();
}

void SystemConfigurator::LoadCartridgeVRAMIfPresent(Emulator& emu) {
    if (emu.cartridge.loaded && !emu.cartridge.vramData.empty()) {
        emu.gpu.LoadVRAM(emu.cartridge.vramData);
    }
}

void SystemConfigurator::MountSDCardIfPresent(Emulator& emu) {
    if (emu.cartridge.loaded && !emu.cartridge.sdCardPath.empty()) {
        if (!emu.sdcard.IsMounted() || emu.sdcard.GetMountedPath() != emu.cartridge.sdCardPath) {
            emu.sdcard.Mount(emu.cartridge.sdCardPath);
        }
        return;
    }

    if (emu.currentBinPath.empty()) {
        return;
    }

    std::filesystem::path sdPath = emu.currentBinPath;
    sdPath.replace_extension(".sd");
    if (std::filesystem::exists(sdPath)) {
        if (!emu.sdcard.IsMounted() || emu.sdcard.GetMountedPath() != sdPath.string()) {
            emu.sdcard.Mount(sdPath.string());
        }
    }
}

void SystemConfigurator::SetupHardware(Emulator& emu) {
    if (emu.engine.IsRunning() && std::this_thread::get_id() != emu.engine.GetThreadId() && !emu.engine.IsActuallyPaused()) {
        emu.WaitUntilSafeToMutate();
    }
    emu.engine.SetHalted(false);
    emu.bus.ClearDevices();
    emu.bus.Init();
    emu.watchpointWriteHookInstalled = false;
    emu.cpu.SetProfiler(&emu.subroutineProfiler);
    emu.cpu.Reset();
    emu.pendingIRQ.store(false, std::memory_order_relaxed);
    emu.pendingNMI.store(false, std::memory_order_relaxed);
    emu.pendingInterruptAny.store(false, std::memory_order_relaxed);
    emu.totalCycles = 0;
    emu.totalInstructions = 0;
    emu.savestateManager.ClearRewindBuffer();

    try {
        if (!RegisterCartridgeLayout(emu)) {
            RegisterDefaultLayout(emu);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during SetupHardware: " << e.what() << '\n';
        if (emu.cartridge.loaded) {
            emu.cartridge.loaded = false;
            SetupHardware(emu);
            return;
        }
    }

    ResetHardwareDevices(emu);

    LoadCartridgeVRAMIfPresent(emu);
    MountSDCardIfPresent(emu);

    emu.via.SetPortBCallback([&emu](Byte val) { emu.HandleVIAPortB(val); });
    emu.EnsureWatchpointWriteHook();
}

} // namespace Core

