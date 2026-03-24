#include "Hardware/Core/CartridgeLoader.h"
#include <miniz.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <cstring>

namespace Core {

bool CartridgeLoader::Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (mz_zip_reader_init_file(&zip_archive, path.c_str(), 0) == MZ_FALSE) {
        errorMsg = "Failed to open ZIP file: " + path;
        return false;
    }

    // Read manifest.json
    size_t manifestSize = 0;
    void* manifestData = mz_zip_reader_extract_file_to_heap(&zip_archive, "manifest.json", &manifestSize, 0);
    if (manifestData == nullptr) {
        mz_zip_reader_end(&zip_archive);
        errorMsg = "Cartridge is missing manifest.json";
        return false;
    }

    std::string manifestStr(static_cast<const char*>(manifestData), manifestSize);
    mz_free(manifestData);

    try {
        auto manifestJson = nlohmann::json::parse(manifestStr);
        
        // Metadata
        if (manifestJson.contains("metadata")) {
            auto metadataObj = manifestJson["metadata"];
            outCartridge.metadata.name = metadataObj.value("name", "Unknown");
            outCartridge.metadata.author = metadataObj.value("author", "Unknown");
            outCartridge.metadata.description = metadataObj.value("description", "");
            outCartridge.metadata.version = metadataObj.value("version", "1.0");
        }

        // Config
        if (manifestJson.contains("config")) {
            auto configObj = manifestJson["config"];
            if (configObj.contains("target_ips")) {
                outCartridge.config.targetIPS = configObj["target_ips"].get<int>();
            }
            if (configObj.contains("gpu_enabled")) {
                outCartridge.config.gpuEnabled = configObj["gpu_enabled"].get<bool>();
            }
            if (configObj.contains("cycle_accurate")) {
                outCartridge.config.cycleAccurate = configObj["cycle_accurate"].get<bool>();
            }
            if (configObj.contains("sid_enabled")) {
                outCartridge.config.sidEnabled = configObj["sid_enabled"].get<bool>();
            }
        }

        // ROM file name
        outCartridge.romFileName = manifestJson.value("rom", "rom.bin");

        // Read ROM data
        size_t romSize = 0;
        void* romData = mz_zip_reader_extract_file_to_heap(&zip_archive, outCartridge.romFileName.c_str(), &romSize, 0);
        if (romData == nullptr) {
            mz_zip_reader_end(&zip_archive);
            errorMsg = "Cartridge is missing ROM file: " + outCartridge.romFileName;
            return false;
        }

        outCartridge.romData.resize(romSize);
        std::memcpy(outCartridge.romData.data(), romData, romSize);
        mz_free(romData);

        outCartridge.loaded = true;
    } catch (const std::exception& e) {
        mz_zip_reader_end(&zip_archive);
        errorMsg = "Failed to parse manifest.json: " + std::string(e.what());
        return false;
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}

} // namespace Core
