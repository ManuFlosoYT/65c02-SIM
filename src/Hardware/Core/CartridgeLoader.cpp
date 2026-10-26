#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/CartridgeManifestParser.h"
#include "Hardware/Core/CartridgeZipIO.h"

namespace Core {

bool CartridgeLoader::Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg) {
    return CartridgeZipIO::Load(path, outCartridge, errorMsg);
}

bool CartridgeLoader::LoadFromMemory(const uint8_t* data, size_t size, Cartridge& outCartridge, std::string& errorMsg) {
    return CartridgeZipIO::LoadFromMemory(data, size, outCartridge, errorMsg);
}

bool CartridgeLoader::SaveSDToZip(const Cartridge& cart) {
    return CartridgeZipIO::SaveSDToZip(cart);
}

void CartridgeLoader::ParseMetadata(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    CartridgeManifestParser::ParseMetadata(manifestJson, outCartridge);
}

void CartridgeLoader::ParseConfig(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    CartridgeManifestParser::ParseConfig(manifestJson, outCartridge);
}

void CartridgeLoader::ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    CartridgeManifestParser::ParseBus(manifestJson, outCartridge);
}

void CartridgeLoader::ValidateBusRequirements(const std::vector<DeviceConfig>& busDevices) {
    CartridgeManifestParser::ValidateBusRequirements(busDevices);
}

DeviceConfig CartridgeLoader::ParseDeviceConfig(const nlohmann::json& devJson) {
    return CartridgeManifestParser::ParseDeviceConfig(devJson);
}

uint16_t CartridgeLoader::ParseAddress(const nlohmann::json& jsonObj, const std::string& key) {
    return CartridgeManifestParser::ParseAddress(jsonObj, key);
}

bool CartridgeLoader::ReadRomData(void* zip_archive, const std::string& romFileName, Cartridge& outCartridge,
                                  std::string& errorMsg) {
    return CartridgeZipIO::ReadRomData(zip_archive, romFileName, outCartridge, errorMsg);
}

bool CartridgeLoader::ReadVramData(void* zip_archive, const std::string& vramFileName, Cartridge& outCartridge,
                                   std::string& errorMsg) {
    return CartridgeZipIO::ReadVramData(zip_archive, vramFileName, outCartridge, errorMsg);
}

bool CartridgeLoader::LoadInternal(void* zip_archive, Cartridge& outCartridge, std::string& errorMsg) {
    return CartridgeZipIO::LoadInternal(zip_archive, outCartridge, errorMsg);
}

}  // namespace Core
