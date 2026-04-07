#pragma once

#include <string>

namespace Control {

class SDKManager {
public:
    // Extracts the bundled SDK.zip to the current directory
    // Overwrites existing files if conflict occurs
    static void ExtractBundledSDK();

    // Scan extracted directories to populate AppState
    static void ScanExtractedSDK(struct AppState& state);

private:
    static bool ExtractFile(const void* pZip, int file_index, const std::string& output_path);
};

} // namespace Control
