#pragma once

#include <filesystem>

namespace Control {

struct AppState;

class SDKCatalogScanner {
public:
    static void ScanExtractedSDK(AppState& state, const std::filesystem::path& sdkDir);
    static void ScanExtractedSDK(AppState& state);
};

} // namespace Control
