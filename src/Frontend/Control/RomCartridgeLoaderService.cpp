#include "Frontend/Control/RomCartridgeLoaderService.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Console.h"
#include "Hardware/Core/Logger.h"

namespace Control {

bool LoadBinaryRom(AppState& state, const std::string& path, const std::vector<uint8_t>& data) {
    state.emulator.Pause();
    std::string errorMsg;
    bool success = false;
    if (!data.empty()) {
        state.rom.data = data;
        success = state.emulator.InitFromMemory(state.rom.data, path, errorMsg);
    } else {
        success = state.emulator.Init(path, errorMsg);
    }

    if (!success) {
        Core::Logger::Error("Failed to load ROM: {}", errorMsg);
        return false;
    }

    Console::Clear();
    state.rom.bin = path;
    state.rom.loaded = true;
    state.rom.symbols.Clear();
    state.emulator.ClearProfiler();
    state.emulator.ClearCartridge();
    return true;
}

bool LoadCartridge(AppState& state, const std::string& path, const std::vector<uint8_t>& data) {
    state.emulator.Pause();
    std::string errorMsg;
    Core::Cartridge cart;
    bool loaded = false;
    if (!data.empty()) {
        loaded = Core::CartridgeLoader::LoadFromMemory(data.data(), data.size(), cart, errorMsg);
    } else {
        loaded = Core::CartridgeLoader::Load(path, cart, errorMsg);
    }

    if (!loaded) {
        Core::Logger::Error("Failed to load cartridge: {}", errorMsg);
        return false;
    }

    Control::ApplyCartridgeConfig(state, cart);
    if (!state.emulator.InitFromMemory(cart.romData, cart.romFileName, errorMsg)) {
        Core::Logger::Error("Failed to init cartridge ROM: {}", errorMsg);
        return false;
    }

    state.rom.bin = path;
    state.rom.loaded = true;
    state.emulator.ClearProfiler();
    return true;
}

}  // namespace Control
