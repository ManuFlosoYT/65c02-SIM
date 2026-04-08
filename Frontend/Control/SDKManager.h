#pragma once

#include <string>

namespace Control {

class SDKManager {
public:
    // Checks if the SDK is present and up to date
    static void CheckSDKStatus(struct AppState& state);

    // Extracts the bundled SDK.zip to the current directory
    static void ExtractBundledSDK();

    // Deletes the entire SDK directory
    static void DeleteSDK();

    // Scan extracted directories to populate AppState
    static void ScanExtractedSDK(struct AppState& state);

private:
    static void WriteManifest();
    static std::string GetExtractedVersion();
};

} // namespace Control
