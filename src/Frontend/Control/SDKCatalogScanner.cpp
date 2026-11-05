#include "Frontend/Control/SDKCatalogScanner.h"
#include "Frontend/Control/SDKInstaller.h"
#include "Frontend/Control/AppState.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include "Hardware/Core/Logger.h"

namespace fs = std::filesystem;

namespace Control {

void SDKCatalogScanner::ScanExtractedSDK(AppState& state, const fs::path& sdkDir) {
    state.sdk.roms.clear();
    state.sdk.midis.clear();
    state.sdk.vrams.clear();

    auto scan_dir = [](const fs::path& dir, std::vector<std::string>& list, const std::string& ext) {
        if (!fs::exists(dir)) {
            return;
        }
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                list.push_back(entry.path().filename().string());
            }
        }
        std::ranges::sort(list);
    };

    scan_dir(sdkDir / "cartridge", state.sdk.roms, ".65c");
    scan_dir(sdkDir / "midi", state.sdk.midis, ".65c");
    scan_dir(sdkDir / "vram", state.sdk.vrams, ".65c");

    if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
        Core::Logger::Warn("SDKManager: No SDK resources found in SDK/ subfolders");
    } else {
        Core::Logger::Info("SDKManager: Found {} ROMs, {} MIDIs, {} VRAMs",
                           state.sdk.roms.size(), state.sdk.midis.size(), state.sdk.vrams.size());
    }

    state.sdk.loaded = !state.sdk.roms.empty() || !state.sdk.midis.empty() || !state.sdk.vrams.empty();
}

void SDKCatalogScanner::ScanExtractedSDK(AppState& state) {
    ScanExtractedSDK(state, SDKInstaller::GetSDKDir());
}

} // namespace Control
