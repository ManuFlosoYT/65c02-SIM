#include "Hardware/Core/CartridgeLoader.h"

#include <miniz.h>

#include <cstring>
#include <nlohmann/json.hpp>
#include <vector>

namespace gsl {
template <typename T>
using owner = T;
}

namespace {

bool ReplaceFileWithBackup(const std::filesystem::path& sourceZipPath, const std::filesystem::path& tempZipPath) {
    std::filesystem::path backupZipPath = sourceZipPath;
    backupZipPath += ".bak";

    std::error_code errorCode;
    std::filesystem::remove(backupZipPath, errorCode);

    errorCode.clear();
    if (std::filesystem::exists(sourceZipPath)) {
        std::filesystem::rename(sourceZipPath, backupZipPath, errorCode);
        if (errorCode) {
            std::filesystem::remove(tempZipPath);
            return false;
        }
    }

    errorCode.clear();
    std::filesystem::rename(tempZipPath, sourceZipPath, errorCode);
    if (errorCode) {
        if (std::filesystem::exists(backupZipPath)) {
            std::error_code restoreErrorCode;
            std::filesystem::rename(backupZipPath, sourceZipPath, restoreErrorCode);
        }
        std::filesystem::remove(tempZipPath);
        return false;
    }

    std::filesystem::remove(backupZipPath, errorCode);
    return true;
}

}  // namespace

namespace Core {

bool CartridgeLoader::Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (mz_zip_reader_init_file(&zip_archive, path.c_str(), 0) == MZ_FALSE) {
        errorMsg = "Failed to open ZIP file: " + path;
        return false;
    }

    outCartridge.sourceZipPath = path;
    bool success = LoadInternal(&zip_archive, outCartridge, errorMsg);
    mz_zip_reader_end(&zip_archive);
    return success;
}

bool CartridgeLoader::LoadFromMemory(const uint8_t* data, size_t size, Cartridge& outCartridge, std::string& errorMsg) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (mz_zip_reader_init_mem(&zip_archive, data, size, 0) == MZ_FALSE) {
        errorMsg = "Failed to open ZIP from memory";
        return false;
    }

    bool success = LoadInternal(&zip_archive, outCartridge, errorMsg);
    mz_zip_reader_end(&zip_archive);
    return success;
}

bool CartridgeLoader::LoadInternal(void* zip_archive_ptr, Cartridge& outCartridge, std::string& errorMsg) {
    auto* zip_archive = static_cast<mz_zip_archive*>(zip_archive_ptr);

    // Read manifest.json
    size_t manifestSize = 0;
    void* manifestData = mz_zip_reader_extract_file_to_heap(zip_archive, "manifest.json", &manifestSize, 0);
    if (manifestData == nullptr) {
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

        outCartridge.version = manifestJson.value("version", "1.0");
        outCartridge.romFileName = manifestJson.value("rom", "");
        outCartridge.vramFileName = manifestJson.value("vram", "");

        if (!outCartridge.romFileName.empty()) {
            if (!ReadRomData(zip_archive, outCartridge.romFileName, outCartridge, errorMsg)) {
                return false;
            }
        }

        if (!outCartridge.vramFileName.empty()) {
            if (!ReadVramData(zip_archive, outCartridge.vramFileName, outCartridge, errorMsg)) {
                return false;
            }
        }

        // Must have either ROM or VRAM
        if (outCartridge.romFileName.empty() && outCartridge.vramFileName.empty()) {
            errorMsg = "Cartridge is missing ROM/VRAM data";
            return false;
        }

        // Look for sdcard.img if version >= 3.0 or just generally for convenience
        size_t sdSize = 0;
        void* sdData = mz_zip_reader_extract_file_to_heap(zip_archive, "sdcard.img", &sdSize, 0);
        if (sdData != nullptr) {
            std::string tempPath;
#ifdef TARGET_WASM
            tempPath = "/tmp/sdcard.img";
#else
            tempPath = std::filesystem::temp_directory_path().string() + "/65c02sim_" +
                       std::filesystem::path(outCartridge.sourceZipPath).filename().string() + ".sdcard.img";
#endif
            gsl::owner<FILE*> sdFile = fopen(tempPath.c_str(), "wb");
            if (sdFile != nullptr) {
                if (fwrite(sdData, 1, sdSize, sdFile) != sdSize) {
                    // Handle write error
                }
                if (fclose(sdFile) != 0) {
                    // Handle close error
                }
                outCartridge.sdCardPath = tempPath;
            }
            mz_free(sdData);
        }

        outCartridge.loaded = true;
    } catch (const std::exception& e) {
        errorMsg = "Failed to parse manifest.json: " + std::string(e.what());
        return false;
    }

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
        if (configObj.contains("target_ips") && configObj["target_ips"].is_number()) {
            outCartridge.config.targetIPS = configObj["target_ips"].get<int>();
        }
        if (configObj.contains("gpu_enabled") && configObj["gpu_enabled"].is_boolean()) {
            outCartridge.config.gpuEnabled = configObj["gpu_enabled"].get<bool>();
        }
        if (configObj.contains("cycle_accurate") && configObj["cycle_accurate"].is_boolean()) {
            outCartridge.config.cycleAccurate = configObj["cycle_accurate"].get<bool>();
        }
        if (configObj.contains("sid_enabled") && configObj["sid_enabled"].is_boolean()) {
            outCartridge.config.sidEnabled = configObj["sid_enabled"].get<bool>();
        }
        if (configObj.contains("esp_enabled") && configObj["esp_enabled"].is_boolean()) {
            outCartridge.config.espEnabled = configObj["esp_enabled"].get<bool>();
        }
        if (configObj.contains("sd_enabled") && configObj["sd_enabled"].is_boolean()) {
            outCartridge.config.sdEnabled = configObj["sd_enabled"].get<bool>();
        }
    }
}

void CartridgeLoader::ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    if (!manifestJson.contains("bus") || !manifestJson["bus"].is_array()) {
        return;
    }

    outCartridge.busDevices.clear();
    for (const auto& devJson : manifestJson["bus"]) {
        outCartridge.busDevices.push_back(ParseDeviceConfig(devJson));
    }

    ValidateBusRequirements(outCartridge.busDevices);
}

uint16_t CartridgeLoader::ParseAddress(const nlohmann::json& jsonObj, const std::string& key) {
    if (jsonObj.contains(key)) {
        if (jsonObj[key].is_number()) {
            return jsonObj[key].get<uint16_t>();
        }
        if (jsonObj[key].is_string()) {
            try {
                return static_cast<uint16_t>(std::stoul(jsonObj[key].get<std::string>(), nullptr, 0));
            } catch (...) {
                throw std::runtime_error("Invalid address format for " + key);
            }
        }
    }
    throw std::runtime_error("Missing address field: " + key);
}

DeviceConfig CartridgeLoader::ParseDeviceConfig(const nlohmann::json& devJson) {
    DeviceConfig dev;
    dev.name = devJson.value("name", "");
    if (dev.name.empty()) {
        throw std::runtime_error("Device missing name in bus configuration");
    }

    // Validate against known list
    if (dev.name != "RAM" && dev.name != "ROM" && dev.name != "VIA" && dev.name != "LCD" &&
        dev.name != "ESP32" && dev.name != "ESP8266" && dev.name != "ACIA" && dev.name != "GPU" && dev.name != "SID" && 
        dev.name != "SD Card") {
        throw std::runtime_error("Unknown device name: " + dev.name);
    }

    dev.start = ParseAddress(devJson, "start");
    dev.end = ParseAddress(devJson, "end");

    if (dev.start > dev.end) {
        throw std::runtime_error("Invalid address range for device " + dev.name + ": start > end");
    }

    return dev;
}

void CartridgeLoader::ValidateBusRequirements(const std::vector<DeviceConfig>& busDevices) {
    bool hasRAM = false;
    bool hasROM = false;
    static constexpr uint32_t kMemoryDeviceMaxSize = 0x8000;

    for (const auto& dev : busDevices) {
        uint32_t rangeSize = static_cast<uint32_t>(dev.end) - static_cast<uint32_t>(dev.start) + 1;
        if (dev.name == "RAM") {
            hasRAM = true;
            if (rangeSize > kMemoryDeviceMaxSize) {
                throw std::runtime_error("Manifest RAM range exceeds 32KB raw RAM size");
            }
        } else if (dev.name == "ROM") {
            hasROM = true;
            if (rangeSize > kMemoryDeviceMaxSize) {
                throw std::runtime_error("Manifest ROM range exceeds 32KB raw ROM size");
            }
        }
    }

    if (!hasRAM) {
        throw std::runtime_error("Manifest bus configuration is missing RAM device");
    }
    if (!hasROM) {
        throw std::runtime_error("Manifest bus configuration is missing ROM device");
    }
}

bool CartridgeLoader::ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge,
                                  std::string& errorMsg) {
    size_t romSize = 0;
    void* romData =
        mz_zip_reader_extract_file_to_heap(static_cast<mz_zip_archive*>(zip_archive), romFileName.c_str(), &romSize, 0);
    if (romData == nullptr) {
        errorMsg = "Cartridge is missing ROM file: " + romFileName;
        return false;
    }

    outCartridge.romData.resize(romSize);
    std::memcpy(outCartridge.romData.data(), romData, romSize);
    mz_free(romData);
    return true;
}

bool CartridgeLoader::ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge,
                                   std::string& errorMsg) {
    size_t vramSize = 0;
    void* vramData = mz_zip_reader_extract_file_to_heap(static_cast<mz_zip_archive*>(zip_archive), vramFileName.c_str(),
                                                        &vramSize, 0);
    if (vramData == nullptr) {
        errorMsg = "Cartridge is missing VRAM file: " + vramFileName;
        return false;
    }

    outCartridge.vramData.resize(vramSize);
    std::memcpy(outCartridge.vramData.data(), vramData, vramSize);
    mz_free(vramData);
    return true;
}

bool CartridgeLoader::SaveSDToZip(const Cartridge& cart) {
    if (cart.sdCardPath.empty() || cart.sourceZipPath.empty()) {
        return false;
    }

    // Miniz doesn't support easy in-place updates of a single file in a large ZIP efficiently
    // without rebuilding. We'll use a temporary file.
    std::filesystem::path sourceZipPath(cart.sourceZipPath);
    std::filesystem::path tempZipPath = sourceZipPath;
    tempZipPath += ".tmp";
    std::string tempZip = tempZipPath.string();
    mz_zip_archive src_archive;
    mz_zip_archive dst_archive;
    memset(&src_archive, 0, sizeof(src_archive));
    memset(&dst_archive, 0, sizeof(dst_archive));

    if (mz_zip_reader_init_file(&src_archive, cart.sourceZipPath.c_str(), 0) == MZ_FALSE) {
        return false;
    }

    if (mz_zip_writer_init_file(&dst_archive, tempZip.c_str(), 0) == MZ_FALSE) {
        mz_zip_reader_end(&src_archive);
        return false;
    }

    bool sdUpdated = false;
    mz_uint numFiles = mz_zip_reader_get_num_files(&src_archive);
    for (mz_uint i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat;
        if (mz_zip_reader_file_stat(&src_archive, i, &file_stat) == MZ_FALSE) {
            continue;
        }

        if (strcmp(file_stat.m_filename, "sdcard.img") == 0) {
            // Updated file from temp path
            if (mz_zip_writer_add_file(&dst_archive, "sdcard.img", cart.sdCardPath.c_str(), nullptr, 0,
                                       MZ_BEST_COMPRESSION) == MZ_FALSE) {
                mz_zip_reader_end(&src_archive);
                mz_zip_writer_end(&dst_archive);
                std::filesystem::remove(tempZip);
                return false;
            }
            sdUpdated = true;
        } else {
            // Copy existing file
            if (mz_zip_writer_add_from_zip_reader(&dst_archive, &src_archive, i) == MZ_FALSE) {
                mz_zip_reader_end(&src_archive);
                mz_zip_writer_end(&dst_archive);
                std::filesystem::remove(tempZip);
                return false;
            }
        }
    }

    if (!sdUpdated) {
        // If sdcard.img wasn't in original but we have it now (shouldn't happen in this flow but just in case)
        if (mz_zip_writer_add_file(&dst_archive, "sdcard.img", cart.sdCardPath.c_str(), nullptr, 0,
                                   MZ_BEST_COMPRESSION) == MZ_FALSE) {
            mz_zip_reader_end(&src_archive);
            mz_zip_writer_end(&dst_archive);
            std::filesystem::remove(tempZip);
            return false;
        }
    }

    mz_zip_reader_end(&src_archive);
    if (mz_zip_writer_finalize_archive(&dst_archive) == MZ_FALSE) {
        mz_zip_writer_end(&dst_archive);
        std::filesystem::remove(tempZipPath);
        return false;
    }
    mz_zip_writer_end(&dst_archive);

    return ReplaceFileWithBackup(sourceZipPath, tempZipPath);
}

}  // namespace Core
