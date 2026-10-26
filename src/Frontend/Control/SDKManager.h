#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Control {

struct AppState;
enum class IDEMode : std::uint8_t;

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

    // Resolves path to a library file (returns empty string if not found)
    static std::string ResolveLibPath(const std::string& lib);

    // Returns available SDK header libraries for a given IDE mode
    static std::vector<std::string> GetAvailableLibraries(IDEMode mode);

private:
    static void WriteManifest();
    static std::string GetExtractedVersion();
};

} // namespace Control
