#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Hardware/Audio/SID.h"

namespace GUI::MIDI {

inline constexpr std::uint8_t SID_WAVE_GATE = 0x01;
inline constexpr std::uint8_t SID_WAVE_SYNC = 0x02;
inline constexpr std::uint8_t SID_WAVE_RING = 0x04;
inline constexpr std::uint8_t SID_WAVE_TEST = 0x08;
inline constexpr std::uint8_t SID_WAVE_TRI = 0x10;
inline constexpr std::uint8_t SID_WAVE_SAW = 0x20;
inline constexpr std::uint8_t SID_WAVE_PULSE = 0x40;
inline constexpr std::uint8_t SID_WAVE_NOISE = 0x80;

enum class SIDFeatureFlags : std::uint16_t {
    None = 0,
    PWMSweep = 1U << 0U,
    Vibrato = 1U << 1U,
    BassFilter = 1U << 2U,
    VelocityFilter = 1U << 3U,
    FilterHP = 1U << 4U,
    FilterBP = 1U << 5U,
    HardSync = 1U << 6U,
    RingMod = 1U << 7U,
    KickSlide = 1U << 8U,
    PCMSample = 1U << 9U,
    SnareMacro = 1U << 10U
};

struct SIDPatchParams {
    std::string name = "Piano";
    std::uint8_t waveMask = SID_WAVE_PULSE;
    std::uint8_t attackDecay = 0x09;
    std::uint8_t sustainReleaseBase = 0x05;
    std::uint8_t effectiveSustainRelease = 0xC5;
    std::uint16_t pulseWidth = 0x0600;
    std::uint16_t features = 0;
    float maxDrumLengthSec = 0.15F;
    int drumPriorityRank = 0;
};

struct MelodicPresetInfo {
    int programStart = 0;
    int programEnd = 7;
    int representativeProgram = 0;
    std::string name;
    SIDPatchParams baseParams;
};

struct PercussionKitInfo {
    std::vector<int> notes;
    std::string name;
    std::string drumClass;
    SIDPatchParams baseParams;
};

struct WaveformOption {
    std::string label;
    std::uint8_t waveMask = 0;
};

class MIDISoundBank {
   public:
    static void EnsureLoaded();
    static bool ReloadFromJsonFile(const std::string& jsonPath);

    static SIDPatchParams GetInstrumentParams(int program, int channel, int note, int velocity,
                                              Hardware::SIDModel chip = Hardware::SIDModel::MOS8580,
                                              std::uint8_t customWaveOverride = 0);

    static const std::vector<MelodicPresetInfo>& GetMelodicPresets();
    static const std::vector<PercussionKitInfo>& GetPercussionKits();
    static const std::vector<WaveformOption>& GetWaveformOptions();

    static std::string GetProgramName(int program);
    static std::string GetPercussionNoteName(int note);
    static std::string FormatWaveformMask(std::uint8_t waveMask);
    static std::vector<std::string> FormatFeatureList(std::uint16_t features);
};

}  // namespace GUI::MIDI

