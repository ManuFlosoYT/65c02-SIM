#pragma once

#include "Frontend/Control/AppState.h"
#include <filesystem>

namespace Control {

class ConfigManager {
public:
    static void Init(AppState& state);
    static bool Load(AppState& state);
    static bool Save(const AppState& state);
    static void CheckAutoReload(AppState& state);
    static std::filesystem::path GetConfigPath();
};

}  // namespace Control
