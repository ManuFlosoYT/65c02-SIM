#pragma once

namespace Core {

class Emulator;
struct DeviceConfig;

class SystemConfigurator {
public:
    static void SetupHardware(Emulator& emulator);
    static bool RegisterCartridgeLayout(Emulator& emulator);
    static void RegisterDefaultLayout(Emulator& emulator);
    static void RegisterCartridgeDevice(Emulator& emulator, const DeviceConfig& dev, bool& sdCustomMapped);
    static void ResetHardwareDevices(Emulator& emulator);
    static void LoadCartridgeVRAMIfPresent(Emulator& emulator);
    static void MountSDCardIfPresent(Emulator& emulator);
};

} // namespace Core

