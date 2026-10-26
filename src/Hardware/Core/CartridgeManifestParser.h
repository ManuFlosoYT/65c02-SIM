#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Hardware/Core/Cartridge.h"

namespace Core {

class CartridgeManifestParser {
public:
    static bool IsMemoryDeviceName(const std::string& deviceName) {
        return deviceName == "RAM" || deviceName == "ROM";
    }

    template <typename RangeCollection>
    static void ValidateNoInvalidOverlaps(const RangeCollection& ranges) {
        using RangeType = typename RangeCollection::value_type;
        const RangeType* lastMemory = nullptr;
        const RangeType* lastMMIO = nullptr;

        for (const auto& range : ranges) {
            bool isMemory = IsMemoryDeviceName(range.name);
            const RangeType*& prev = isMemory ? lastMemory : lastMMIO;
            if (prev != nullptr && range.start <= prev->end) {
                throw std::runtime_error("Manifest bus configuration contains overlapping devices: " +
                                         prev->name + " and " + range.name);
            }
            prev = &range;
        }
    }

    static void ParseMetadata(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static void ParseConfig(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static void ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge);
    static void ValidateBusRequirements(const std::vector<DeviceConfig>& busDevices);
    static DeviceConfig ParseDeviceConfig(const nlohmann::json& devJson);
    static uint16_t ParseAddress(const nlohmann::json& jsonObj, const std::string& key);
    static std::optional<Hardware::SIDModel> ParseSIDModelJson(const nlohmann::json& jsonVal);
};

}  // namespace Core
