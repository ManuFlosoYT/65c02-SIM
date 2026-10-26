#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "Frontend/Control/AppState.h"

namespace Control {

class CartridgeExporter {
public:
    static std::string Base64UrlEncode(const uint8_t* data, size_t length);
    static nlohmann::json BuildBusJson(const AppState& state);
    static nlohmann::json GenerateManifest(const AppState& state,
                                           const std::string& name,
                                           const std::string& author,
                                           const std::string& description,
                                           const std::string& version,
                                           int sidModel);

    static bool ExportCartridgeToFile(const AppState& state,
                                      const std::string& manifestStr,
                                      const std::string& outPath);

    static bool ExportCartridgeToUrl(AppState& state,
                                     const std::string& manifestStr);
};

} // namespace Control
