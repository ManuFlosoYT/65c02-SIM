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
};

struct Cartridge {
    CartridgeMetadata metadata;
    CartridgeConfig config;
    std::vector<uint8_t> romData;
    std::string romFileName;
    bool loaded = false;
};

} // namespace Core
