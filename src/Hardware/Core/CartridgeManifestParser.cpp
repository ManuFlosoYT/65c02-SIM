#include "Hardware/Core/CartridgeManifestParser.h"

#include <algorithm>
#include <unordered_map>

namespace Core {

std::optional<Hardware::SIDModel> CartridgeManifestParser::ParseSIDModelJson(const nlohmann::json& jsonVal) {
    std::string modelStr;
    if (jsonVal.is_string()) {
        modelStr = jsonVal.get<std::string>();
    } else if (jsonVal.is_number()) {
        modelStr = std::to_string(jsonVal.get<int>());
    } else {
        return std::nullopt;
    }

    if (modelStr.find("6581") != std::string::npos) {
        return Hardware::SIDModel::MOS6581;
    }
    if (modelStr.find("8580") != std::string::npos) {
        return Hardware::SIDModel::MOS8580;
    }
    return std::nullopt;
}

void CartridgeManifestParser::ParseMetadata(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    if (manifestJson.contains("metadata")) {
        const auto& metadataObj = manifestJson["metadata"];
        outCartridge.metadata.name = metadataObj.value("name", "Unknown");
        outCartridge.metadata.author = metadataObj.value("author", "Unknown");
        outCartridge.metadata.description = metadataObj.value("description", "");
        outCartridge.metadata.version = metadataObj.value("version", "1.0");
    }
}

void CartridgeManifestParser::ParseConfig(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
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
        if (configObj.contains("sid_model")) {
            auto model = ParseSIDModelJson(configObj["sid_model"]);
            if (model.has_value()) {
                outCartridge.config.sidModel = model;
            }
        }
    }
}

void CartridgeManifestParser::ParseBus(const nlohmann::json& manifestJson, Cartridge& outCartridge) {
    if (!manifestJson.contains("bus") || !manifestJson["bus"].is_array()) {
        return;
    }

    outCartridge.busDevices.clear();
    for (const auto& devJson : manifestJson["bus"]) {
        outCartridge.busDevices.push_back(ParseDeviceConfig(devJson));
        if (!outCartridge.config.sidModel.has_value() && devJson.is_object() && devJson.value("name", "") == "SID") {
            if (devJson.contains("model")) {
                auto model = ParseSIDModelJson(devJson["model"]);
                if (model.has_value()) {
                    outCartridge.config.sidModel = model;
                }
            }
            if (!outCartridge.config.sidModel.has_value() && devJson.contains("sid_model")) {
                auto model = ParseSIDModelJson(devJson["sid_model"]);
                if (model.has_value()) {
                    outCartridge.config.sidModel = model;
                }
            }
        }
    }

    ValidateBusRequirements(outCartridge.busDevices);
}

uint16_t CartridgeManifestParser::ParseAddress(const nlohmann::json& jsonObj, const std::string& key) {
    if (jsonObj.contains(key)) {
        if (jsonObj[key].is_number()) {
            uint32_t parsed = jsonObj[key].get<uint32_t>();
            if (parsed > 0xFFFFU) {
                throw std::runtime_error("Address out of range for " + key + ": " + std::to_string(parsed));
            }
            return static_cast<uint16_t>(parsed);
        }
        if (jsonObj[key].is_string()) {
            try {
                unsigned long parsed = std::stoul(jsonObj[key].get<std::string>(), nullptr, 0);
                if (parsed > 0xFFFFUL) {
                    throw std::runtime_error("Address out of range for " + key + ": " + std::to_string(parsed));
                }
                return static_cast<uint16_t>(parsed);
            } catch (...) {
                throw std::runtime_error("Invalid address format for " + key);
            }
        }
    }
    throw std::runtime_error("Missing address field: " + key);
}

DeviceConfig CartridgeManifestParser::ParseDeviceConfig(const nlohmann::json& devJson) {
    DeviceConfig dev;
    dev.name = devJson.value("name", "");
    if (dev.name.empty()) {
        throw std::runtime_error("Device missing name in bus configuration");
    }

    if (dev.name != "RAM" && dev.name != "ROM" && dev.name != "VIA" && dev.name != "LCD" &&
        dev.name != "ESP32" && dev.name != "ESP8266" && dev.name != "ACIA" && dev.name != "GPU" && dev.name != "SID" && 
        dev.name != "SD Card" && dev.name != "microDOS-GPU") {
        throw std::runtime_error("Unknown device name: " + dev.name);
    }

    if (dev.name == "LCD") {
        dev.start = devJson.contains("start") ? ParseAddress(devJson, "start") : 0;
        dev.end = devJson.contains("end") ? ParseAddress(devJson, "end") : 0;
        return dev;
    }

    dev.start = ParseAddress(devJson, "start");
    dev.end = ParseAddress(devJson, "end");

    if (dev.start > dev.end) {
        throw std::runtime_error("Invalid address range for device " + dev.name + ": start > end");
    }

    return dev;
}

void CartridgeManifestParser::ValidateBusRequirements(const std::vector<DeviceConfig>& busDevices) {
    bool hasRAM = false;
    bool hasROM = false;

    static const std::unordered_map<std::string, uint32_t> kDeviceMaxRange = {
        {"RAM", 0x8000},
        {"ROM", 0x8000},
        {"ACIA", 0x0004},
        {"VIA", 0x0010},
        {"ESP32", 0x0004},
        {"ESP8266", 0x0004},
        {"SD Card", 0x0004},
        {"SID", 0x0020},
        {"GPU", 0x2000},
        {"microDOS-GPU", 0x2000}
    };

    struct RangeEntry {
        uint16_t start;
        uint16_t end;
        std::string name;
    };

    std::vector<RangeEntry> ranges;
    ranges.reserve(busDevices.size());

    for (const auto& dev : busDevices) {
        if (dev.name == "LCD") {
            continue;
        }

        uint32_t rangeSize = static_cast<uint32_t>(dev.end) - static_cast<uint32_t>(dev.start) + 1;

        if (const auto rangeIt = kDeviceMaxRange.find(dev.name); rangeIt != kDeviceMaxRange.end()) {
            if (rangeSize > rangeIt->second) {
                throw std::runtime_error("Manifest " + dev.name + " range exceeds raw memory/MMIO capacity");
            }
        }

        if (dev.name == "RAM") {
            hasRAM = true;
        } else if (dev.name == "ROM") {
            hasROM = true;
        }

        ranges.push_back({dev.start, dev.end, dev.name});
    }

    std::ranges::sort(ranges, [](const RangeEntry& lhs, const RangeEntry& rhs) { return lhs.start < rhs.start; });
    ValidateNoInvalidOverlaps(ranges);

    if (!hasRAM) {
        throw std::runtime_error("Manifest bus configuration is missing RAM device");
    }
    if (!hasROM) {
        throw std::runtime_error("Manifest bus configuration is missing ROM device");
    }
}

}  // namespace Core
