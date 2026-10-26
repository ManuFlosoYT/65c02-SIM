#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include "Hardware/Core/Cartridge.h"

namespace Core {

class CartridgeZipIO {
public:
    static bool Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg);
    static bool LoadFromMemory(const uint8_t* data, size_t size, Cartridge& outCartridge, std::string& errorMsg);
    static bool SaveSDToZip(const Cartridge& cart);

    static bool ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge, std::string& errorMsg);
    static bool ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge, std::string& errorMsg);
    static bool ExtractDbgData(void* zip_archive, Cartridge& outCartridge);
    static bool ExtractSDCard(void* zip_archive, Cartridge& outCartridge, std::string& errorMsg);
    static bool ReplaceFileWithBackup(const std::filesystem::path& sourceZipPath, const std::filesystem::path& tempZipPath);
    static bool LoadInternal(void* zip_archive_ptr, Cartridge& outCartridge, std::string& errorMsg);
};

}  // namespace Core
