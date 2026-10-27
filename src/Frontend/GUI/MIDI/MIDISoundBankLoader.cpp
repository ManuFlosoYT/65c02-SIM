#include "Frontend/GUI/MIDI/MIDISoundBankLoader.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace GUI::MIDI {

namespace {

std::uint8_t ParseWaveName(const std::string& waveName) {
    if (waveName == "TRI") {
        return SID_WAVE_TRI;
    }
    if (waveName == "SAW") {
        return SID_WAVE_SAW;
    }
    if (waveName == "PULSE") {
        return SID_WAVE_PULSE;
    }
    if (waveName == "NOISE") {
        return SID_WAVE_NOISE;
    }
    return 0;
}

std::uint16_t ParseFeatureName(const std::string& featureName) {
    if (featureName == "pwm_sweep") {
        return FlagBit(SIDFeatureFlags::PWMSweep);
    }
    if (featureName == "vibrato") {
        return FlagBit(SIDFeatureFlags::Vibrato);
    }
    if (featureName == "bass_filter") {
        return FlagBit(SIDFeatureFlags::BassFilter);
    }
    if (featureName == "velocity_filter") {
        return FlagBit(SIDFeatureFlags::VelocityFilter);
    }
    if (featureName == "filter_hp") {
        return FlagBit(SIDFeatureFlags::FilterHP);
    }
    if (featureName == "filter_bp") {
        return FlagBit(SIDFeatureFlags::FilterBP);
    }
    if (featureName == "hard_sync") {
        return FlagBit(SIDFeatureFlags::HardSync);
    }
    if (featureName == "ring_mod") {
        return FlagBit(SIDFeatureFlags::RingMod);
    }
    if (featureName == "kick_slide") {
        return FlagBit(SIDFeatureFlags::KickSlide);
    }
    if (featureName == "pcm_sample") {
        return FlagBit(SIDFeatureFlags::PCMSample);
    }
    if (featureName == "snare_macro") {
        return FlagBit(SIDFeatureFlags::SnareMacro);
    }
    return 0;
}

std::uint16_t ParseHexValue(const std::string& str, std::uint16_t fallback) {
    if (str.empty()) {
        return fallback;
    }
    char* endPtr = nullptr;
    const unsigned long val = std::strtoul(str.c_str(), &endPtr, 0);
    if (endPtr == str.c_str()) {
        return fallback;
    }
    return static_cast<std::uint16_t>(val & 0xFFFFU);
}

int ParseDecimalInt(const std::string& str, int fallback = 0) {
    if (str.empty()) {
        return fallback;
    }
    char* endPtr = nullptr;
    const long val = std::strtol(str.c_str(), &endPtr, 10);
    if (endPtr == str.c_str()) {
        return fallback;
    }
    return static_cast<int>(val);
}

}  // namespace

bool LoadSoundBankFromJson(const std::string& jsonPath, SoundBankStorage& storage) {
    std::ifstream inFile(jsonPath);
    if (!inFile.is_open()) {
        return false;
    }

    const nlohmann::json root = nlohmann::json::parse(inFile, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return false;
    }

    if (root.contains("melodic") && root["melodic"].is_object()) {
        std::vector<MelodicPresetInfo> parsedMelodic;
        for (const auto& [key, val] : root["melodic"].items()) {
            if (!val.is_object()) {
                continue;
            }
            int startProg = 0;
            int endProg = 0;
            const std::size_t dashPos = key.find('-');
            if (dashPos != std::string::npos) {
                startProg = ParseDecimalInt(key.substr(0, dashPos));
                endProg = ParseDecimalInt(key.substr(dashPos + 1));
            } else {
                startProg = ParseDecimalInt(key);
                endProg = startProg;
            }

            std::uint8_t waveMask = 0;
            if (val.contains("waves") && val["waves"].is_array()) {
                for (const auto& w : val["waves"]) {
                    if (w.is_string()) {
                        waveMask |= ParseWaveName(w.get<std::string>());
                    }
                }
            }
            if (waveMask == 0) {
                waveMask = SID_WAVE_PULSE;
            }

            std::uint16_t features = 0;
            if (val.contains("features") && val["features"].is_array()) {
                for (const auto& f : val["features"]) {
                    if (f.is_string()) {
                        features |= ParseFeatureName(f.get<std::string>());
                    }
                }
            }

            const std::string name = val.value("name", key);
            const auto ad =
                static_cast<std::uint8_t>(ParseHexValue(val.value("attack_decay", "0x09"), 0x09));
            const auto srBase = static_cast<std::uint8_t>(
                ParseHexValue(val.value("sustain_release_base", "0x05"), 0x05));
            const std::uint16_t pw = ParseHexValue(val.value("pulse_width", "0x0800"), 0x0800);

            parsedMelodic.push_back(
                MakeMelodicPreset(startProg, endProg, name, waveMask, ad, srBase, pw, features));
        }

        if (!parsedMelodic.empty()) {
            std::sort(parsedMelodic.begin(), parsedMelodic.end(),
                      [](const MelodicPresetInfo& a, const MelodicPresetInfo& b) {
                          return a.programStart < b.programStart;
                      });
            storage.melodicPresets = std::move(parsedMelodic);
        }
    }

    if (root.contains("percussion") && root["percussion"].is_object()) {
        std::vector<PercussionKitInfo> parsedPerc;
        for (const auto& [key, val] : root["percussion"].items()) {
            if (!val.is_object()) {
                continue;
            }
            std::vector<int> notes;
            if (key != "default") {
                std::stringstream ss(key);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    if (!token.empty()) {
                        notes.push_back(ParseDecimalInt(token));
                    }
                }
            }

            std::uint8_t waveMask = 0;
            if (val.contains("waves") && val["waves"].is_array()) {
                for (const auto& w : val["waves"]) {
                    if (w.is_string()) {
                        waveMask |= ParseWaveName(w.get<std::string>());
                    }
                }
            }
            if (waveMask == 0) {
                waveMask = SID_WAVE_NOISE;
            }

            std::uint16_t features = 0;
            if (val.contains("features") && val["features"].is_array()) {
                for (const auto& f : val["features"]) {
                    if (f.is_string()) {
                        features |= ParseFeatureName(f.get<std::string>());
                    }
                }
            }

            const std::string name = val.value("name", key);
            const std::string drumClass = val.value("drum_class", "default");
            const float maxLen = val.value("max_len", 0.15F);
            const auto ad =
                static_cast<std::uint8_t>(ParseHexValue(val.value("attack_decay", "0x05"), 0x05));
            const auto srBase = static_cast<std::uint8_t>(
                ParseHexValue(val.value("sustain_release_base", "0x00"), 0x00));
            const std::uint16_t pw = ParseHexValue(val.value("pulse_width", "0x0800"), 0x0800);

            parsedPerc.push_back(MakePercussionKit(std::move(notes), name, drumClass, maxLen,
                                                   waveMask, ad, srBase, pw, features));
        }

        if (!parsedPerc.empty()) {
            std::sort(parsedPerc.begin(), parsedPerc.end(),
                      [](const PercussionKitInfo& a, const PercussionKitInfo& b) {
                          return a.baseParams.drumPriorityRank > b.baseParams.drumPriorityRank;
                      });
            storage.percussionKits = std::move(parsedPerc);
        }
    }

    return true;
}

}  // namespace GUI::MIDI
