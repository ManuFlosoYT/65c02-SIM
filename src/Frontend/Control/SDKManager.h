#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Control {

struct AppState;
enum class IDEMode : std::uint8_t;

class SDKManager {
public:
    static void CheckSDKStatus(struct AppState& state);

    static void ExtractBundledSDK();

    static void DeleteSDK();

    static void ScanExtractedSDK(struct AppState& state);

    static std::string ResolveLibPath(const std::string& lib);

    static std::vector<std::string> GetAvailableLibraries(IDEMode mode);
};

} // namespace Control
