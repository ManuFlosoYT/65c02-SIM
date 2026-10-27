#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Frontend/GUI/MIDI/MIDISoundBank.h"

namespace GUI::MIDI {

struct SoundBankStorage {
    bool initialized = false;
    std::vector<MelodicPresetInfo> melodicPresets;
    std::vector<PercussionKitInfo> percussionKits;
    std::vector<WaveformOption> waveformOptions;
};

inline constexpr std::uint16_t FlagBit(SIDFeatureFlags flag) {
    return static_cast<std::uint16_t>(flag);
}

inline int RankForDrumClass(const std::string& drumClass) {
    if (drumClass == "kick") {
        return 60;
    }
    if (drumClass == "snare") {
        return 50;
    }
    if (drumClass == "crash") {
        return 40;
    }
    if (drumClass == "tom") {
        return 30;
    }
    if (drumClass == "open_hat") {
        return 20;
    }
    if (drumClass == "closed_hat") {
        return 10;
    }
    return 0;
}

inline MelodicPresetInfo MakeMelodicPreset(int startProg, int endProg, std::string name,
                                          std::uint8_t waveMask, std::uint8_t ad, std::uint8_t srBase,
                                          std::uint16_t pw, std::uint16_t features) {
    MelodicPresetInfo preset;
    preset.programStart = startProg;
    preset.programEnd = endProg;
    preset.representativeProgram = startProg;
    preset.name = std::move(name);
    preset.baseParams.name = preset.name;
    preset.baseParams.waveMask = waveMask;
    preset.baseParams.attackDecay = ad;
    preset.baseParams.sustainReleaseBase = srBase;
    preset.baseParams.effectiveSustainRelease =
        (srBase > 0x0FU) ? srBase : static_cast<std::uint8_t>(0xC0U | (srBase & 0x0FU));
    preset.baseParams.pulseWidth = pw;
    preset.baseParams.features = features;
    return preset;
}

inline PercussionKitInfo MakePercussionKit(std::vector<int> notes, std::string name, std::string drumClass,
                                          float maxLen, std::uint8_t waveMask, std::uint8_t ad,
                                          std::uint8_t srBase, std::uint16_t pw, std::uint16_t features) {
    PercussionKitInfo kit;
    kit.notes = std::move(notes);
    kit.name = std::move(name);
    kit.drumClass = std::move(drumClass);
    kit.baseParams.name = kit.name;
    kit.baseParams.waveMask = waveMask;
    kit.baseParams.attackDecay = ad;
    kit.baseParams.sustainReleaseBase = srBase;
    kit.baseParams.effectiveSustainRelease = srBase;
    kit.baseParams.pulseWidth = pw;
    kit.baseParams.features = features;
    kit.baseParams.maxDrumLengthSec = maxLen;
    kit.baseParams.drumPriorityRank = RankForDrumClass(kit.drumClass);
    return kit;
}

bool LoadSoundBankFromJson(const std::string& jsonPath, SoundBankStorage& storage);

}  // namespace GUI::MIDI
