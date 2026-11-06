#include "Frontend/Control/RomCartridgeLoaderService.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Console.h"
#include "Hardware/Core/Logger.h"

namespace Control {

bool LoadBinaryRom(AppState& state, const std::string& path, const std::vector<uint8_t>& data) {
    if (!state.backend) {
        if (!state.CreateBackend(state.cachedSidSampleRate)) {
            return false;
        }
    }
    state.backend->Pause();
    std::string errorMsg;
    bool success = false;
    if (!data.empty()) {
        state.rom.data = data;
        success = state.backend->InitFromMemory(state.rom.data, path, errorMsg);
    } else {
        success = state.backend->Init(path, errorMsg);
    }

    if (!success) {
        Core::Logger::Error("Failed to load ROM: {}", errorMsg);
        state.TriggerBackendError(errorMsg, false);
        return false;
    }

    Console::Clear();
    state.rom.bin = path;
    state.rom.loaded = true;
    state.rom.symbols.Clear();
    state.backend->ClearProfiler();
    state.backend->ClearCartridge();
    return true;
}

bool LoadCartridge(AppState& state, const std::string& path, const std::vector<uint8_t>& data) {
    if (!state.backend) {
        if (!state.CreateBackend(state.cachedSidSampleRate)) {
            return false;
        }
    }
    state.backend->Pause();
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
        state.TriggerBackendError(errorMsg, false);
        return false;
    }

    Control::ApplyCartridgeConfig(state, cart);
    if (!state.backend->InitFromMemory(cart.romData, cart.romFileName, errorMsg)) {
        Core::Logger::Error("Failed to init cartridge ROM: {}", errorMsg);
        state.TriggerBackendError(errorMsg, false);
        return false;
    }

    state.rom.bin = path;
    state.rom.loaded = true;
    state.backend->ClearProfiler();
    return true;
}

}  // namespace Control
