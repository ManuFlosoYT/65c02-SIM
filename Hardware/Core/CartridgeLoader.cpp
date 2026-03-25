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
        ParseMetadata(manifestJson, outCartridge);
        ParseConfig(manifestJson, outCartridge);
        ParseBus(manifestJson, outCartridge);

        outCartridge.romFileName = manifestJson.value("rom", "");
        outCartridge.vramFileName = manifestJson.value("vram", "");

        if (!outCartridge.romFileName.empty()) {
            if (!ReadRomData(&zip_archive, outCartridge.romFileName, outCartridge, errorMsg)) {
                return false;
            }
        }

        if (!outCartridge.vramFileName.empty()) {
            if (!ReadVramData(&zip_archive, outCartridge.vramFileName, outCartridge, errorMsg)) {
                return false;
            }
        }

        // Must have either ROM or VRAM
        if (outCartridge.romFileName.empty() && outCartridge.vramFileName.empty()) {
            mz_zip_reader_end(&zip_archive);
            errorMsg = "Cartridge is missing ROM/VRAM data";
            return false;
        }

        outCartridge.loaded = true;
    } catch (const std::exception& e) {
        mz_zip_reader_end(&zip_archive);
        errorMsg = "Failed to parse manifest.json: " + std::string(e.what());
        return false;
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}

void CartridgeLoader::ParseMetadata(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    if (manifestJson.contains("metadata")) {
        const auto& metadataObj = manifestJson["metadata"];
        outCartridge.metadata.name = metadataObj.value("name", "Unknown");
        outCartridge.metadata.author = metadataObj.value("author", "Unknown");
        outCartridge.metadata.description = metadataObj.value("description", "");
        outCartridge.metadata.version = metadataObj.value("version", "1.0");
    }
}

void CartridgeLoader::ParseConfig(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    if (manifestJson.contains("config")) {
        const auto& configObj = manifestJson["config"];
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
        if (configObj.contains("esp_enabled")) {
            outCartridge.config.espEnabled = configObj["esp_enabled"].get<bool>();
        }
        if (configObj.contains("sd_enabled")) {
            outCartridge.config.sdEnabled = configObj["sd_enabled"].get<bool>();
        }
    }
}

void CartridgeLoader::ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    outCartridge.busDevices.clear();
    if (manifestJson.contains("bus") && manifestJson["bus"].is_array()) {
        auto parseAddr = [](const nlohmann::json& jsonObj, const std::string& key) -> uint16_t {
            if (jsonObj.contains(key)) {
                if (jsonObj[key].is_number()) {
                    return jsonObj[key].get<uint16_t>();
                }
                if (jsonObj[key].is_string()) {
                    return static_cast<uint16_t>(std::stoul(jsonObj[key].get<std::string>(), nullptr, 0));
                }
            }
            return 0;
        };

        for (const auto& devJson : manifestJson["bus"]) {
            DeviceConfig dev;
            dev.name = devJson.value("name", "Unknown");
            dev.start = parseAddr(devJson, "start");
            dev.end = parseAddr(devJson, "end");
            outCartridge.busDevices.push_back(dev);
        }
    }
}

bool CartridgeLoader::ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge, std::string& errorMsg) {
    size_t romSize = 0;
    void* romData = mz_zip_reader_extract_file_to_heap(static_cast<mz_zip_archive*>(zip_archive), romFileName.c_str(), &romSize, 0);
    if (romData == nullptr) {
        mz_zip_reader_end(static_cast<mz_zip_archive*>(zip_archive));
        errorMsg = "Cartridge is missing ROM file: " + romFileName;
        return false;
    }

    outCartridge.romData.resize(romSize);
    std::memcpy(outCartridge.romData.data(), romData, romSize);
    mz_free(romData);
    return true;
}

bool CartridgeLoader::ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge, std::string& errorMsg) {
    size_t vramSize = 0;
    void* vramData = mz_zip_reader_extract_file_to_heap(static_cast<mz_zip_archive*>(zip_archive), vramFileName.c_str(), &vramSize, 0);
    if (vramData == nullptr) {
        mz_zip_reader_end(static_cast<mz_zip_archive*>(zip_archive));
        errorMsg = "Cartridge is missing VRAM file: " + vramFileName;
        return false;
    }

    outCartridge.vramData.resize(vramSize);
    std::memcpy(outCartridge.vramData.data(), vramData, vramSize);
    mz_free(vramData);
    return true;
}

} // namespace Core
