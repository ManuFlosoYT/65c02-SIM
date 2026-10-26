#include "Frontend/Control/SDKCatalogScanner.h"
#include "Frontend/Control/SDKInstaller.h"
#include "Frontend/Control/AppState.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
        std::cerr << "SDKManager: No SDK resources found in SDK/ subfolders\n";
    } else {
        std::cout << "SDKManager: Found " << state.sdk.roms.size() << " ROMs, " << state.sdk.midis.size()
                  << " MIDIs, " << state.sdk.vrams.size() << " VRAMs\n";
    }

    state.sdk.loaded = !state.sdk.roms.empty() || !state.sdk.midis.empty() || !state.sdk.vrams.empty();
}

void SDKCatalogScanner::ScanExtractedSDK(AppState& state) {
    ScanExtractedSDK(state, SDKInstaller::GetSDKDir());
}

} // namespace Control
