#include "Hardware/Core/CartridgeZipIO.h"
#include "Hardware/Core/CartridgeManifestParser.h"

#include <miniz.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

namespace gsl {
template <typename T>
using owner = T;
}

namespace Core {

bool CartridgeZipIO::ReplaceFileWithBackup(const std::filesystem::path& sourceZipPath, const std::filesystem::path& tempZipPath) {
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

bool CartridgeZipIO::Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg) {
    outCartridge = Cartridge{};
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

bool CartridgeZipIO::LoadFromMemory(const uint8_t* data, size_t size, Cartridge& outCartridge, std::string& errorMsg) {
    outCartridge = Cartridge{};
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

bool CartridgeZipIO::LoadInternal(void* zip_archive_ptr, Cartridge& outCartridge, std::string& errorMsg) {
    auto* zip_archive = static_cast<mz_zip_archive*>(zip_archive_ptr);

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
        CartridgeManifestParser::ParseMetadata(manifestJson, outCartridge);
        CartridgeManifestParser::ParseConfig(manifestJson, outCartridge);
        CartridgeManifestParser::ParseBus(manifestJson, outCartridge);

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

        if (outCartridge.romFileName.empty() && outCartridge.vramFileName.empty()) {
            errorMsg = "Cartridge is missing ROM/VRAM data";
            return false;
        }

        if (manifestJson.contains("dbg") && manifestJson["dbg"].is_string()) {
            outCartridge.dbgFileName = manifestJson["dbg"].get<std::string>();
        } else if (manifestJson.contains("elf") && manifestJson["elf"].is_string()) {
            outCartridge.dbgFileName = manifestJson["elf"].get<std::string>();
        } else if (manifestJson.contains("symbols") && manifestJson["symbols"].is_string()) {
            outCartridge.dbgFileName = manifestJson["symbols"].get<std::string>();
        }

        ExtractDbgData(zip_archive, outCartridge);

        if (!ExtractSDCard(zip_archive, outCartridge, errorMsg)) {
            return false;
        }

        outCartridge.loaded = true;
    } catch (const std::exception& e) {
        errorMsg = "Failed to parse manifest.json: " + std::string(e.what());
        return false;
    }

    return true;
}

bool CartridgeZipIO::ExtractDbgData(void* zip_archive_ptr, Cartridge& outCartridge) {
    auto* zip_archive = static_cast<mz_zip_archive*>(zip_archive_ptr);
    if (outCartridge.dbgFileName.empty()) {
        if (mz_zip_reader_locate_file(zip_archive, "rom.dbg", nullptr, 0) >= 0) {
            outCartridge.dbgFileName = "rom.dbg";
        } else if (mz_zip_reader_locate_file(zip_archive, "rom.elf", nullptr, 0) >= 0) {
            outCartridge.dbgFileName = "rom.elf";
        }
    }

    if (!outCartridge.dbgFileName.empty()) {
        size_t dbgSize = 0;
        void* dbgData = mz_zip_reader_extract_file_to_heap(zip_archive, outCartridge.dbgFileName.c_str(), &dbgSize, 0);
        if (dbgData != nullptr) {
            outCartridge.dbgData.resize(dbgSize);
            std::memcpy(outCartridge.dbgData.data(), dbgData, dbgSize);

            std::string tempDbgPath;
#ifdef TARGET_WASM
            tempDbgPath = "/tmp/" + outCartridge.dbgFileName;
#else
            std::string prefix = "cart";
            if (!outCartridge.sourceZipPath.empty()) {
                std::string stem = std::filesystem::path(outCartridge.sourceZipPath).stem().string();
                if (!stem.empty()) {
                    prefix = stem;
                }
            }
            tempDbgPath = std::filesystem::temp_directory_path().string() + "/65c02sim_" + prefix + "_" +
                          outCartridge.dbgFileName;
#endif
            gsl::owner<FILE*> dbgFile = fopen(tempDbgPath.c_str(), "wb");
            if (dbgFile != nullptr) {
                if (fwrite(dbgData, 1, dbgSize, dbgFile) == dbgSize) {
                    outCartridge.dbgFilePath = tempDbgPath;
                }
                fclose(dbgFile);
            }
            mz_free(dbgData);
            return true;
        }
    }
    return false;
}

bool CartridgeZipIO::ExtractSDCard(void* zip_archive_ptr, Cartridge& outCartridge, std::string& errorMsg) {
    auto* zip_archive = static_cast<mz_zip_archive*>(zip_archive_ptr);
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
        if (sdFile == nullptr) {
            mz_free(sdData);
            errorMsg = "Failed to open temporary SD card file for writing";
            return false;
        }
        if (fwrite(sdData, 1, sdSize, sdFile) != sdSize) {
            fclose(sdFile);
            mz_free(sdData);
            errorMsg = "Failed to write temporary SD card file";
            return false;
        }
        if (fclose(sdFile) != 0) {
            mz_free(sdData);
            errorMsg = "Failed to close temporary SD card file";
            return false;
        }
        outCartridge.sdCardPath = tempPath;
        mz_free(sdData);
    }
    return true;
}

bool CartridgeZipIO::ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge,
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

bool CartridgeZipIO::ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge,
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

bool CartridgeZipIO::SaveSDToZip(const Cartridge& cart) {
    if (cart.sdCardPath.empty() || cart.sourceZipPath.empty()) {
        return false;
    }

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
            if (mz_zip_writer_add_file(&dst_archive, "sdcard.img", cart.sdCardPath.c_str(), nullptr, 0,
                                       MZ_BEST_COMPRESSION) == MZ_FALSE) {
                mz_zip_reader_end(&src_archive);
                mz_zip_writer_end(&dst_archive);
                std::filesystem::remove(tempZip);
                return false;
            }
            sdUpdated = true;
        } else {
            if (mz_zip_writer_add_from_zip_reader(&dst_archive, &src_archive, i) == MZ_FALSE) {
                mz_zip_reader_end(&src_archive);
                mz_zip_writer_end(&dst_archive);
                std::filesystem::remove(tempZip);
                return false;
            }
        }
    }

    if (!sdUpdated) {
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
