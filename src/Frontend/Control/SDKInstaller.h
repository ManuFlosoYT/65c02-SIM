#pragma once

#include <filesystem>
#include <string>

namespace Control {

class SDKInstaller {
public:
    static std::filesystem::path GetSDKDir();
    static bool IsInstalled();
    static std::string GetExtractedVersion();
    static void WriteManifest();
    static void ExtractBundledSDK();
    static void DeleteSDK();
};

} // namespace Control
