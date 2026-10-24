#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "Hardware/Audio/SID.h"

namespace GUI::MIDI {

inline constexpr const char* META_ROLE_MELODY = "65C02_ROLE:MELODY";
inline constexpr const char* META_ROLE_BASS = "65C02_ROLE:BASS";
inline constexpr const char* META_ROLE_HARMONY = "65C02_ROLE:HARMONY";

enum class MIDITrackRole : std::uint8_t {
    Auto = 0,
    Melody = 1,
    Bass = 2,
    Harmony = 3,
    Percussion = 4
};

struct MIDINote {
    std::uint32_t id = 0;
    int pitch = 60;
    int velocity = 100;
    double startBeat = 0.0;
    double durationBeats = 1.0;
    bool selected = false;
    int assignedSidVoice = 0;
};

struct MIDITrack {
    std::string name = "Track";
    int channel = 0;
    int program = 0;
    MIDITrackRole role = MIDITrackRole::Auto;
    bool isMainTrack = false;
    bool muted = false;
    bool solo = false;
    std::uint8_t customWaveOverride = 0;
    std::vector<MIDINote> notes;
};

struct MIDIProject {
    std::string filePath;
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    std::uint16_t ticksPerQuarterNote = 480;
    std::vector<MIDITrack> tracks;
    std::uint32_t nextNoteId = 1;

    void InitDefaultProject();
    void SetMainTrack(std::size_t trackIndex);
    [[nodiscard]] int GetMainTrackIndex() const;
    [[nodiscard]] double BeatsToSeconds(double beats) const;
    [[nodiscard]] double SecondsToBeats(double seconds) const;
    [[nodiscard]] double GetTotalDurationBeats() const;
};

class MIDIFile {
   public:
    static bool LoadFromFile(const std::string& path, MIDIProject& outProject, std::string& outError);
    static bool SaveToFile(const std::string& path, const MIDIProject& project, std::string& outError);

    static bool ParseFromBytes(const std::vector<std::uint8_t>& data, MIDIProject& outProject, std::string& outError);
    static std::vector<std::uint8_t> SerializeToBytes(const MIDIProject& project);

    static void ComputeVoicePreviewAllocation(MIDIProject& project,
                                              Hardware::SIDModel chipModel = Hardware::SIDModel::MOS8580);
};

}  // namespace GUI::MIDI

