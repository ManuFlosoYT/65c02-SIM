#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace Core {

struct CartridgeMetadata {
    std::string name;
    std::string author;
    std::string description;
    std::string version;
};

struct CartridgeConfig {
    std::optional<int> targetIPS;
    std::optional<bool> gpuEnabled;
    std::optional<bool> cycleAccurate;
    std::optional<bool> sidEnabled;
    std::optional<bool> espEnabled;
    std::optional<bool> sdEnabled;
};

struct DeviceConfig {
    std::string name;
    uint16_t start;
    uint16_t end;
};

struct Cartridge {
    CartridgeMetadata metadata;
    CartridgeConfig config;
    std::vector<DeviceConfig> busDevices;
    std::vector<uint8_t> romData;
    std::vector<uint8_t> vramData;
    std::string romFileName;
    std::string vramFileName;
    bool loaded = false;
};

} // namespace Core
