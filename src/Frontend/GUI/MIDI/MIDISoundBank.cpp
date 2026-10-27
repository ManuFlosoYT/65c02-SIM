#include "Frontend/GUI/MIDI/MIDISoundBank.h"
#include "Frontend/GUI/MIDI/MIDISoundBankLoader.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace GUI::MIDI {

namespace {

SoundBankStorage& GetStorage() {
    static SoundBankStorage storage;
    return storage;
}

void PopulateDefaultTables(SoundBankStorage& storage) {
    storage.waveformOptions = {
        {"Auto (Instrument Default)", 0},
        {"TRI (0x10)", SID_WAVE_TRI},
        {"SAW (0x20)", SID_WAVE_SAW},
        {"PULSE (0x40)", SID_WAVE_PULSE},
        {"NOISE (0x80)", SID_WAVE_NOISE},
        {"SAW+TRI (0x30)", static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_TRI)},
        {"PULSE+TRI (0x50)", static_cast<std::uint8_t>(SID_WAVE_PULSE | SID_WAVE_TRI)},
        {"SAW+PULSE (0x60)", static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_PULSE)},
        {"SAW+PULSE+TRI (0x70)",
         static_cast<std::uint8_t>(SID_WAVE_SAW | SID_WAVE_PULSE | SID_WAVE_TRI)},
        {"NOISE+TRI (0x90)", static_cast<std::uint8_t>(SID_WAVE_NOISE | SID_WAVE_TRI)},
        {"NOISE+SAW (0xA0)", static_cast<std::uint8_t>(SID_WAVE_NOISE | SID_WAVE_SAW)},
        {"PULSE+NOISE (0xC0)", static_cast<std::uint8_t>(SID_WAVE_PULSE | SID_WAVE_NOISE)},
    };

    const std::uint16_t fPWM = FlagBit(SIDFeatureFlags::PWMSweep);
    const std::uint16_t fVib = FlagBit(SIDFeatureFlags::Vibrato);
    const std::uint16_t fBassF = FlagBit(SIDFeatureFlags::BassFilter);
    const std::uint16_t fVelF = FlagBit(SIDFeatureFlags::VelocityFilter);
    const std::uint16_t fSync = FlagBit(SIDFeatureFlags::HardSync);
    const std::uint16_t fRing = FlagBit(SIDFeatureFlags::RingMod);
    const std::uint16_t fKick = FlagBit(SIDFeatureFlags::KickSlide);
    const std::uint16_t fPCM = FlagBit(SIDFeatureFlags::PCMSample);
    const std::uint16_t fSnare = FlagBit(SIDFeatureFlags::SnareMacro);

    storage.melodicPresets = {
        MakeMelodicPreset(0, 7, "Piano", SID_WAVE_PULSE, 0x09, 0x05, 0x0600, 0),
        MakeMelodicPreset(8, 15, "Chromatic Percussion", SID_WAVE_TRI, 0x05, 0x05, 0x0800, 0),
        MakeMelodicPreset(16, 23, "Organ", SID_WAVE_PULSE | SID_WAVE_TRI, 0x11, 0x02, 0x0400, 0),
        MakeMelodicPreset(24, 28, "Acoustic Guitar", SID_WAVE_SAW, 0x09, 0x04, 0x0800, 0),
        MakeMelodicPreset(29, 31, "Electric/Overdrive Guitar", SID_WAVE_PULSE | SID_WAVE_SAW, 0x09,
                          0x04, 0x0800, fSync),
        MakeMelodicPreset(32, 34, "Acoustic/Electric Bass", SID_WAVE_PULSE, 0x05, 0x04, 0x0800,
                          fBassF | fVelF),
        MakeMelodicPreset(35, 37, "Fretless/Slap Bass", SID_WAVE_SAW, 0x05, 0x04, 0x0800,
                          fBassF | fVelF),
        MakeMelodicPreset(38, 39, "Synth Bass", SID_WAVE_TRI | SID_WAVE_PULSE, 0x05, 0x04, 0x0800,
                          fBassF | fVelF | fPWM),
        MakeMelodicPreset(40, 47, "Strings", SID_WAVE_SAW, 0x68, 0x09, 0x0800, fPWM | fVib),
        MakeMelodicPreset(48, 55, "Ensemble", SID_WAVE_SAW | SID_WAVE_TRI, 0x88, 0x0A, 0x0800,
                          fPWM),
        MakeMelodicPreset(56, 63, "Brass", SID_WAVE_SAW, 0x25, 0x05, 0x0800, fVelF),
        MakeMelodicPreset(64, 71, "Reed", SID_WAVE_TRI | SID_WAVE_PULSE, 0x45, 0x06, 0x0800, fVib),
        MakeMelodicPreset(72, 79, "Pipe", SID_WAVE_TRI, 0x56, 0x06, 0x0800, fVib),
        MakeMelodicPreset(80, 80, "Synth Lead (Square)", SID_WAVE_PULSE | SID_WAVE_TRI, 0x05, 0x09,
                          0x0800, fPWM | fVib),
        MakeMelodicPreset(81, 81, "Synth Lead (Sawtooth)", SID_WAVE_SAW | SID_WAVE_TRI, 0x05, 0x09,
                          0x0800, fPWM | fVib | fRing),
        MakeMelodicPreset(82, 87, "Synth Lead (Other)", SID_WAVE_PULSE | SID_WAVE_TRI, 0x05, 0x09,
                          0x0800, fPWM | fVib),
        MakeMelodicPreset(88, 95, "Synth Pad", SID_WAVE_SAW | SID_WAVE_TRI, 0xAA, 0x0A, 0x0800,
                          fPWM),
        MakeMelodicPreset(96, 103, "Synth Effects", SID_WAVE_PULSE | SID_WAVE_NOISE, 0x2A, 0x0A,
                          0x0800, fPWM),
        MakeMelodicPreset(104, 111, "Ethnic", SID_WAVE_TRI, 0x05, 0x06, 0x0800, 0),
        MakeMelodicPreset(112, 119, "Percussive", SID_WAVE_PULSE | SID_WAVE_TRI, 0x01, 0x04, 0x0800,
                          0),
        MakeMelodicPreset(120, 127, "Sound Effects", SID_WAVE_NOISE, 0x00, 0x00, 0x0800, 0),
        MakeMelodicPreset(128, 128, "APU Pulse", SID_WAVE_PULSE, 0x00, 0x00, 0x0800, 0),
        MakeMelodicPreset(129, 129, "APU Triangle", SID_WAVE_TRI, 0x00, 0x00, 0x0800, 0),
        MakeMelodicPreset(130, 130, "APU Noise", SID_WAVE_NOISE, 0x00, 0x00, 0x0800, 0),
    };

    storage.percussionKits = {
        MakePercussionKit({35, 36}, "Kick", "kick", 0.15F, SID_WAVE_TRI, 0x05, 0x00, 0x0800,
                          fKick | fPCM),
        MakePercussionKit({37, 38, 40}, "Snare", "snare", 0.15F, SID_WAVE_NOISE | SID_WAVE_TRI,
                          0x05, 0x00, 0x0800, fSnare | fPCM),
        MakePercussionKit({49, 51, 52, 53, 55, 57, 59}, "Crash & Cymbals", "crash", 0.25F,
                          SID_WAVE_NOISE, 0x0F, 0x08, 0x0800, 0),
        MakePercussionKit({41, 43, 45, 47, 48, 50}, "Toms", "tom", 0.15F, SID_WAVE_TRI, 0x05, 0x00,
                          0x0800, 0),
        MakePercussionKit({46}, "Open Hi-Hat", "open_hat", 0.12F, SID_WAVE_NOISE, 0x00, 0x04,
                          0x0800, 0),
        MakePercussionKit({42, 44}, "Closed Hi-Hat", "closed_hat", 0.06F, SID_WAVE_NOISE, 0x00,
                          0x02, 0x0800, 0),
        MakePercussionKit({}, "Generic Percussion", "default", 0.10F, SID_WAVE_NOISE, 0x05, 0x00,
                          0x0800, 0),
    };
}

}  // namespace

void MIDISoundBank::EnsureLoaded() {
    SoundBankStorage& storage = GetStorage();
    if (storage.initialized) {
        return;
    }
    storage.initialized = true;
    PopulateDefaultTables(storage);

    std::error_code ec;
    if (std::filesystem::exists("tools/sid/instruments.json", ec)) {
        ReloadFromJsonFile("tools/sid/instruments.json");
    } else if (std::filesystem::exists("../tools/sid/instruments.json", ec)) {
        ReloadFromJsonFile("../tools/sid/instruments.json");
    }
}

bool MIDISoundBank::ReloadFromJsonFile(const std::string& jsonPath) {
    EnsureLoaded();
    return LoadSoundBankFromJson(jsonPath, GetStorage());
}

SIDPatchParams MIDISoundBank::GetInstrumentParams(int program, int channel, int note, int velocity,
                                                  Hardware::SIDModel chip,
                                                  std::uint8_t customWaveOverride) {
    EnsureLoaded();
    const SoundBankStorage& storage = GetStorage();
    const bool isDrum = (channel == 9);

    SIDPatchParams params;
    bool found = false;

    if (isDrum) {
        const PercussionKitInfo* defaultKit = nullptr;
        for (const auto& kit : storage.percussionKits) {
            if (kit.notes.empty()) {
                defaultKit = &kit;
                continue;
            }
            if (std::find(kit.notes.begin(), kit.notes.end(), note) != kit.notes.end()) {
                params = kit.baseParams;
                found = true;
                break;
            }
        }
        if (!found && defaultKit != nullptr) {
            params = defaultKit->baseParams;
            found = true;
        }
    } else {
        for (const auto& preset : storage.melodicPresets) {
            if (program >= preset.programStart && program <= preset.programEnd) {
                params = preset.baseParams;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        params.name = "Default Pulse";
        params.waveMask = SID_WAVE_PULSE;
        params.attackDecay = 0x09;
        params.sustainReleaseBase = 0x05;
        params.pulseWidth = 0x0800;
        params.features = 0;
    }

    if (customWaveOverride != 0) {
        params.waveMask = customWaveOverride;
    }

    if (!isDrum) {
        const int clampedVel = std::clamp(velocity, 0, 127);
        const int susNibble =
            std::clamp(static_cast<int>((static_cast<double>(clampedVel) / 127.0) * 15.0), 4, 15);
        if (params.sustainReleaseBase > 0x0FU) {
            params.effectiveSustainRelease = params.sustainReleaseBase;
        } else {
            params.effectiveSustainRelease = static_cast<std::uint8_t>(
                (static_cast<unsigned int>(susNibble) << 4U) | (params.sustainReleaseBase & 0x0FU));
        }
    } else {
        params.effectiveSustainRelease = params.sustainReleaseBase;
    }

    if (chip != Hardware::SIDModel::MOS6581) {
        params.features =
            static_cast<std::uint16_t>(params.features & ~FlagBit(SIDFeatureFlags::PCMSample));
    }

    if (chip == Hardware::SIDModel::MOS6581) {
        if ((params.waveMask & SID_WAVE_NOISE) != 0 && params.waveMask != SID_WAVE_NOISE) {
            params.waveMask = SID_WAVE_NOISE;
        }
    }

    return params;
}

const std::vector<MelodicPresetInfo>& MIDISoundBank::GetMelodicPresets() {
    EnsureLoaded();
    return GetStorage().melodicPresets;
}

const std::vector<PercussionKitInfo>& MIDISoundBank::GetPercussionKits() {
    EnsureLoaded();
    return GetStorage().percussionKits;
}

const std::vector<WaveformOption>& MIDISoundBank::GetWaveformOptions() {
    EnsureLoaded();
    return GetStorage().waveformOptions;
}

std::string MIDISoundBank::GetProgramName(int program) {
    EnsureLoaded();
    for (const auto& preset : GetStorage().melodicPresets) {
        if (program >= preset.programStart && program <= preset.programEnd) {
            return preset.name;
        }
    }
    return "Program " + std::to_string(program);
}

std::string MIDISoundBank::GetPercussionNoteName(int note) {
    EnsureLoaded();
    const PercussionKitInfo* defaultKit = nullptr;
    for (const auto& kit : GetStorage().percussionKits) {
        if (kit.notes.empty()) {
            defaultKit = &kit;
            continue;
        }
        if (std::find(kit.notes.begin(), kit.notes.end(), note) != kit.notes.end()) {
            return kit.name;
        }
    }
    if (defaultKit != nullptr) {
        return defaultKit->name;
    }
    return "Generic Percussion";
}

std::string MIDISoundBank::FormatWaveformMask(std::uint8_t waveMask) {
    if (waveMask == 0) {
        return "AUTO";
    }
    std::string result;
    const auto appendWave = [&result](const char* label) {
        if (!result.empty()) {
            result += "+";
        }
        result += label;
    };

    if ((waveMask & SID_WAVE_SAW) != 0) {
        appendWave("SAW");
    }
    if ((waveMask & SID_WAVE_PULSE) != 0) {
        appendWave("PULSE");
    }
    if ((waveMask & SID_WAVE_NOISE) != 0) {
        appendWave("NOISE");
    }
    if ((waveMask & SID_WAVE_TRI) != 0) {
        appendWave("TRI");
    }

    if (result.empty()) {
        std::ostringstream oss;
        oss << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(waveMask);
        return oss.str();
    }
    return result;
}

std::vector<std::string> MIDISoundBank::FormatFeatureList(std::uint16_t features) {
    std::vector<std::string> list;
    if ((features & FlagBit(SIDFeatureFlags::PWMSweep)) != 0) {
        list.emplace_back("pwm_sweep");
    }
    if ((features & FlagBit(SIDFeatureFlags::Vibrato)) != 0) {
        list.emplace_back("vibrato");
    }
    if ((features & FlagBit(SIDFeatureFlags::BassFilter)) != 0) {
        list.emplace_back("bass_filter");
    }
    if ((features & FlagBit(SIDFeatureFlags::VelocityFilter)) != 0) {
        list.emplace_back("velocity_filter");
    }
    if ((features & FlagBit(SIDFeatureFlags::FilterHP)) != 0) {
        list.emplace_back("filter_hp");
    }
    if ((features & FlagBit(SIDFeatureFlags::FilterBP)) != 0) {
        list.emplace_back("filter_bp");
    }
    if ((features & FlagBit(SIDFeatureFlags::HardSync)) != 0) {
        list.emplace_back("hard_sync");
    }
    if ((features & FlagBit(SIDFeatureFlags::RingMod)) != 0) {
        list.emplace_back("ring_mod");
    }
    if ((features & FlagBit(SIDFeatureFlags::KickSlide)) != 0) {
        list.emplace_back("kick_slide");
    }
    if ((features & FlagBit(SIDFeatureFlags::PCMSample)) != 0) {
        list.emplace_back("pcm_sample");
    }
    if ((features & FlagBit(SIDFeatureFlags::SnareMacro)) != 0) {
        list.emplace_back("snare_macro");
    }
    return list;
}

}  // namespace GUI::MIDI
