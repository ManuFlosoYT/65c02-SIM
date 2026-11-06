#include "Frontend/Control/ConfigManager.h"
#include "Frontend/Control/ConfigPlatform.h"
#include "Frontend/Platform/PlatformUtils.h"
#include "Hardware/Core/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

namespace Control {

static std::string_view Trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.remove_suffix(1);
    }
    return s;
}

static std::string ToLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

static bool ParseBool(std::string_view s, bool defaultVal) {
    auto low = ToLower(Trim(s));
    if (low == "true" || low == "1" || low == "on" || low == "yes") {
        return true;
    }
    if (low == "false" || low == "0" || low == "off" || low == "no") {
        return false;
    }
    return defaultVal;
}

static float ParseFloat(std::string_view s, float defaultVal) {
    try {
        std::string str(Trim(s));
        if (str.empty()) {
            return defaultVal;
        }
        std::replace(str.begin(), str.end(), ',', '.');
        return std::stof(str);
    } catch (...) {
        return defaultVal;
    }
}

static GUI::CRTMonitorType ParseMonitorType(std::string_view s, GUI::CRTMonitorType defaultVal) {
    auto low = ToLower(Trim(s));
    if (low == "rgb" || low == "0") {
        return GUI::CRTMonitorType::RGB;
    }
    if (low == "green p1" || low == "green" || low == "1") {
        return GUI::CRTMonitorType::GreenP1;
    }
    if (low == "amber p3" || low == "amber" || low == "2") {
        return GUI::CRTMonitorType::AmberP3;
    }
    if (low == "white p4" || low == "white" || low == "3") {
        return GUI::CRTMonitorType::WhiteP4;
    }
    return defaultVal;
}

static const char* MonitorTypeToString(GUI::CRTMonitorType type) {
    switch (type) {
        case GUI::CRTMonitorType::RGB: return "RGB";
        case GUI::CRTMonitorType::GreenP1: return "Green P1";
        case GUI::CRTMonitorType::AmberP3: return "Amber P3";
        case GUI::CRTMonitorType::WhiteP4: return "White P4";
        default: return "RGB";
    }
}

std::filesystem::path ConfigManager::GetConfigPath() {
    return Frontend::Platform::GetExecutableDirectory() / "config.ini";
}

void ConfigManager::Init(AppState& state) {
    std::error_code ec;
    auto configPath = GetConfigPath();
    if (std::filesystem::exists(configPath, ec)) {
        if (Load(state)) {
            Core::Logger::Info("Loaded configuration from {}", configPath.string());
        }
    } else {
        if (Save(state)) {
            Core::Logger::Info("Created default configuration at {}", configPath.string());
        }
    }
}

bool ConfigManager::Save(const AppState& state) {
    auto configPath = GetConfigPath();
    std::ofstream ofs(configPath);
    if (!ofs.is_open()) {
        Core::Logger::Warn("Failed to open {} for writing", configPath.string());
        return false;
    }

    ofs << "[Emulation]\n";
    ofs << "cycle_accurate = " << (state.emulation.cycleAccurate ? "true" : "false") << "\n";
    ofs << "force_load_savestate = " << (state.emulation.forceLoadSaveState ? "true" : "false") << "\n";
    ofs << "auto_reload = " << (state.emulation.autoReload ? "true" : "false") << "\n";
    ofs << "sd_latency = " << (state.emulation.sdLatencyEnabled ? "true" : "false") << "\n";
    ofs << "ram_power_on_pattern = " << (state.emulation.ramPowerOnPattern ? "true" : "false") << "\n";
    ofs << "sid_phase_trigger = " << (state.emulation.sidPhaseTrigger ? "true" : "false") << "\n";
    ofs << "sid_adsr_warning = " << (state.emulation.sidADSRWarning ? "true" : "false") << "\n";
    ofs << "vram_pixel_inspector = " << (state.emulation.vramPixelInspector ? "true" : "false") << "\n\n";

    ofs << "[CRT]\n";
    ofs << "scanlines = " << (state.crt.scanlines ? "true" : "false") << "\n";
    ofs << "interlacing = " << (state.crt.interlacing ? "true" : "false") << "\n";
    ofs << "curvature = " << (state.crt.curvature ? "true" : "false") << "\n";
    ofs << "chromatic = " << (state.crt.chromatic ? "true" : "false") << "\n";
    ofs << "blur = " << (state.crt.blur ? "true" : "false") << "\n";
    ofs << "shadow_mask = " << (state.crt.shadowMask ? "true" : "false") << "\n";
    ofs << "vignette = " << (state.crt.vignette ? "true" : "false") << "\n";
    ofs << "corner_rounding = " << (state.crt.cornerRounding ? "true" : "false") << "\n";
    ofs << "glass_glare = " << (state.crt.glassGlare ? "true" : "false") << "\n";
    ofs << "color_bleeding = " << (state.crt.colorBleeding ? "true" : "false") << "\n";
    ofs << "noise = " << (state.crt.noise ? "true" : "false") << "\n";
    ofs << "vsync_jitter = " << (state.crt.vsyncJitter ? "true" : "false") << "\n";
    ofs << "phosphor_decay = " << (state.crt.phosphorDecay ? "true" : "false") << "\n";
    ofs << "bloom = " << (state.crt.bloom ? "true" : "false") << "\n";
    ofs << "halation = " << (state.crt.halation ? "true" : "false") << "\n";
    ofs << "ghosting = " << (state.crt.ghosting ? "true" : "false") << "\n";
    ofs << "moire = " << (state.crt.moire ? "true" : "false") << "\n";
    ofs << "# Range: 1.00 - 3.50\n";
    ofs << "gamma = " << std::format("{:.2f}", state.crt.gamma) << "\n";
    ofs << "sharp_pixels = " << (state.crt.sharpPixels ? "true" : "false") << "\n";
    ofs << "hum_bar = " << (state.crt.humBar ? "true" : "false") << "\n";
    ofs << "# Options: RGB, Green P1, Amber P3, White P4\n";
    ofs << "monitor_type = " << MonitorTypeToString(state.crt.monitorType) << "\n";
    ofs << "raster_beam = " << (state.crt.rasterBeam ? "true" : "false") << "\n";

    ofs.close();

    ConfigPlatform::UpdateFileTimestamp(configPath);
    return true;
}

bool ConfigManager::Load(AppState& state) {
    auto configPath = GetConfigPath();
    std::ifstream ifs(configPath);
    if (!ifs.is_open()) {
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(ifs, line)) {
        auto trimmed = Trim(line);
        if (trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with(';')) {
            continue;
        }

        if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
            currentSection = ToLower(Trim(trimmed.substr(1, trimmed.size() - 2)));
            continue;
        }

        auto eqPos = trimmed.find('=');
        if (eqPos == std::string_view::npos) {
            continue;
        }

        auto key = ToLower(Trim(trimmed.substr(0, eqPos)));
        auto val = Trim(trimmed.substr(eqPos + 1));

        if (currentSection == "emulation") {
            if (key == "cycle_accurate") {
                state.emulation.cycleAccurate = ParseBool(val, state.emulation.cycleAccurate);
                if (state.backend) {
                    bool cycleOverridden = state.backend->GetCartridge().config.cycleAccurate.has_value();
                    if (!cycleOverridden) {
                        state.backend->SetCycleAccurate(state.emulation.cycleAccurate);
                    }
                }
            } else if (key == "force_load_savestate") {
                state.emulation.forceLoadSaveState = ParseBool(val, state.emulation.forceLoadSaveState);
            } else if (key == "auto_reload") {
                state.emulation.autoReload = ParseBool(val, state.emulation.autoReload);
                ConfigPlatform::SetAutoReload(state, state.emulation.autoReload);
            } else if (key == "sd_latency") {
                state.emulation.sdLatencyEnabled = ParseBool(val, state.emulation.sdLatencyEnabled);
                if (state.backend) {
                    state.backend->GetSDCard().SetReadLatencyEnabled(state.emulation.sdLatencyEnabled);
                }
            } else if (key == "ram_power_on_pattern") {
                state.emulation.ramPowerOnPattern = ParseBool(val, state.emulation.ramPowerOnPattern);
                if (state.backend) {
                    state.backend->GetRAM().SetPowerOnPattern(state.emulation.ramPowerOnPattern);
                }
            } else if (key == "sid_phase_trigger") {
                state.emulation.sidPhaseTrigger = ParseBool(val, state.emulation.sidPhaseTrigger);
            } else if (key == "sid_adsr_warning") {
                state.emulation.sidADSRWarning = ParseBool(val, state.emulation.sidADSRWarning);
            } else if (key == "vram_pixel_inspector") {
                state.emulation.vramPixelInspector = ParseBool(val, state.emulation.vramPixelInspector);
            }
        } else if (currentSection == "crt") {
            if (key == "scanlines") {
                state.crt.scanlines = ParseBool(val, state.crt.scanlines);
            } else if (key == "interlacing") {
                state.crt.interlacing = ParseBool(val, state.crt.interlacing);
            } else if (key == "curvature") {
                state.crt.curvature = ParseBool(val, state.crt.curvature);
            } else if (key == "chromatic") {
                state.crt.chromatic = ParseBool(val, state.crt.chromatic);
            } else if (key == "blur") {
                state.crt.blur = ParseBool(val, state.crt.blur);
            } else if (key == "shadow_mask") {
                state.crt.shadowMask = ParseBool(val, state.crt.shadowMask);
            } else if (key == "vignette") {
                state.crt.vignette = ParseBool(val, state.crt.vignette);
            } else if (key == "corner_rounding") {
                state.crt.cornerRounding = ParseBool(val, state.crt.cornerRounding);
            } else if (key == "glass_glare") {
                state.crt.glassGlare = ParseBool(val, state.crt.glassGlare);
            } else if (key == "color_bleeding") {
                state.crt.colorBleeding = ParseBool(val, state.crt.colorBleeding);
            } else if (key == "noise") {
                state.crt.noise = ParseBool(val, state.crt.noise);
            } else if (key == "vsync_jitter") {
                state.crt.vsyncJitter = ParseBool(val, state.crt.vsyncJitter);
            } else if (key == "phosphor_decay") {
                state.crt.phosphorDecay = ParseBool(val, state.crt.phosphorDecay);
            } else if (key == "bloom") {
                state.crt.bloom = ParseBool(val, state.crt.bloom);
            } else if (key == "halation") {
                state.crt.halation = ParseBool(val, state.crt.halation);
            } else if (key == "ghosting") {
                state.crt.ghosting = ParseBool(val, state.crt.ghosting);
            } else if (key == "moire") {
                state.crt.moire = ParseBool(val, state.crt.moire);
            } else if (key == "gamma") {
                state.crt.gamma = ParseFloat(val, state.crt.gamma);
            } else if (key == "sharp_pixels") {
                state.crt.sharpPixels = ParseBool(val, state.crt.sharpPixels);
            } else if (key == "hum_bar") {
                state.crt.humBar = ParseBool(val, state.crt.humBar);
            } else if (key == "monitor_type") {
                state.crt.monitorType = ParseMonitorType(val, state.crt.monitorType);
            } else if (key == "raster_beam") {
                state.crt.rasterBeam = ParseBool(val, state.crt.rasterBeam);
                if (state.backend) {
                    state.backend->SetRasterBeamSlowMotion(state.crt.rasterBeam);
                }
            }
        }
    }

    ConfigPlatform::UpdateFileTimestamp(configPath);
    return true;
}

void ConfigManager::CheckAutoReload(AppState& state) {
    auto configPath = GetConfigPath();
    if (ConfigPlatform::CheckFileModified(configPath)) {
        Core::Logger::Info("External change detected in {}. Reloading...", configPath.string());
        Load(state);
    }
}

}  // namespace Control
