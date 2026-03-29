#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "Hardware/Core/Cartridge.h"

namespace Core {

class CartridgeLoader {
public:
    static bool Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg);
    static bool LoadFromMemory(const uint8_t* data, size_t size, Cartridge& outCartridge, std::string& errorMsg);
    static bool SaveSDToZip(const Cartridge& cart);

private:
    static void ParseMetadata(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static void ParseConfig(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static void ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static bool ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge, std::string& errorMsg);
    static bool ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge, std::string& errorMsg);
    static bool LoadInternal(void* zip_archive, Cartridge& outCartridge, std::string& errorMsg);
};

} // namespace Core
