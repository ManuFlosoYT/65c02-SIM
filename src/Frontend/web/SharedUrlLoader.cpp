#include "Frontend/web/SharedUrlLoader.h"
#include "Frontend/web/BrowserFileBridge.h"

#ifdef TARGET_WASM
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>
#include <miniz.h>
#include "Hardware/Core/Logger.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Console.h"

namespace {
Control::AppState* g_appState = nullptr;
}
#endif

namespace Frontend {

void SharedUrlLoader::Init(Control::AppState* state) {
#ifdef TARGET_WASM
    g_appState = state;
    CheckUrlParameters();
#else
    (void)state;
#endif
}

void SharedUrlLoader::CheckUrlParameters() {
#ifdef TARGET_WASM
    BrowserFileBridge::CheckUrlHash();
#endif
}

bool SharedUrlLoader::LoadPayload(const uint8_t* data, size_t size) {
#ifdef TARGET_WASM
    if (g_appState == nullptr || data == nullptr || size == 0) {
        return false;
    }
    auto& state = *g_appState;

    std::vector<uint8_t> uncompressed;
    const uint8_t* payloadData = data;
    size_t payloadSize = size;

    if (size > 4) {
        uint32_t origSize = static_cast<uint32_t>(data[0]) |
                            (static_cast<uint32_t>(data[1]) << 8) |
                            (static_cast<uint32_t>(data[2]) << 16) |
                            (static_cast<uint32_t>(data[3]) << 24);
        if (origSize > 0 && origSize <= 8 * 1024 * 1024) {
            uncompressed.resize(origSize);
            mz_ulong destLen = origSize;
            if (mz_uncompress(uncompressed.data(), &destLen, data + 4, static_cast<mz_ulong>(size - 4)) == MZ_OK &&
                destLen == origSize) {
                payloadData = uncompressed.data();
                payloadSize = uncompressed.size();
            }
        }
    }

    if (!state.backend) {
        state.CreateBackend(state.cachedSidSampleRate);
    }
    if (!state.backend) {
        return false;
    }

    if (payloadSize >= 4 && payloadData[0] == 'P' && payloadData[1] == 'K' &&
        payloadData[2] == 0x03 && payloadData[3] == 0x04) {
        state.backend->Pause();
        std::string errorMsg;
        Core::Cartridge cart;
        if (Core::CartridgeLoader::LoadFromMemory(payloadData, payloadSize, cart, errorMsg)) {
            Control::ApplyCartridgeConfig(state, cart);
            if (state.backend->InitFromMemory(cart.romData, cart.romFileName, errorMsg)) {
                state.rom.bin = cart.metadata.name.empty() ? "shared_demo.65c" : cart.metadata.name;
                state.rom.loaded = true;
                state.backend->ClearProfiler();
                state.backend->Resume();
                Console::Clear();
                Core::Logger::Info("Loaded shared cartridge: {}", state.rom.bin);
                BrowserFileBridge::ShowToast("Loaded demo from URL!");
                return true;
            }
        }
    }

    if (payloadSize >= 11 && memcmp(payloadData, "SIM65C02SST", 11) == 0) {
        std::string virtualPath = "/tmp/shared_demo.savestate";
        FILE* f = fopen(virtualPath.c_str(), "wb");
        if (f != nullptr) {
            fwrite(payloadData, 1, payloadSize, f);
            fclose(f);
            state.backend->Pause();
            if (state.backend->LoadState(virtualPath, true)) {
                Control::SyncUIStateFromEmulator(state);
                state.backend->Resume();
                Core::Logger::Info("Loaded shared savestate.");
                BrowserFileBridge::ShowToast("Loaded state from URL!");
                return true;
            }
        }
    }

    std::span<const uint8_t> dataSpan(payloadData, payloadSize);
    state.rom.data.assign(dataSpan.begin(), dataSpan.end());
    state.backend->Pause();
    std::string errorMsg;
    if (state.backend->InitFromMemory(state.rom.data, "shared_demo.bin", errorMsg)) {
        Console::Clear();
        state.rom.bin = "shared_demo.bin";
        state.rom.loaded = true;
        state.rom.symbols.Clear();
        state.backend->ClearProfiler();
        state.backend->ClearCartridge();
        state.backend->Resume();
        Core::Logger::Info("Loaded shared ROM.");
        BrowserFileBridge::ShowToast("Loaded ROM from URL!");
        return true;
    }

    return false;
#else
    (void)data;
    (void)size;
    return false;
#endif
}

}  // namespace Frontend
