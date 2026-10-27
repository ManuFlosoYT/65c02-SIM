#include "Frontend/Control/SDUtils.h"
#include "Hardware/Storage/FAT16Utils.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include <filesystem>
#include <fstream>

namespace GUI {

bool IsSDCardEnabled(Control::AppState& state) {
    auto& bus = state.emulator.GetMem();
    const auto& devices = bus.GetRegisteredDevices();
    for (const auto& reg : devices) {
        if (reg.device == static_cast<Hardware::IBusDevice*>(&state.emulator.GetSDCard())) {
            return reg.enabled;
        }
    }
    return false;
}

bool CreateFAT16Image(const std::string& path) {
    return Hardware::Storage::CreateFAT16Image(path);
}

bool MountFolder(Control::AppState& state, const std::string& folderPath, bool allowDeletion) {
    if (!IsSDCardEnabled(state)) {
        return false;
    }
    return state.emulator.GetSDCard().MountFolder(folderPath, allowDeletion);
}

bool CreateFAT16FromFolder(const std::string& folderPath, const std::string& outputPath) {
    return Hardware::Storage::CreateFAT16FromFolder(folderPath, outputPath);
}

bool CreateMountSentinel(const std::string& folderPath) {
    try {
        std::filesystem::path sentinelPath = std::filesystem::path(std::u8string(folderPath.begin(), folderPath.end())) / ".65C02_SIM_mount";
        std::ofstream sentinel(sentinelPath);
        return sentinel.good();
    } catch (...) {
        return false;
    }
}

}  // namespace GUI
