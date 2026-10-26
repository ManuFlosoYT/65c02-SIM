#pragma once

#include <array>
#include <cstdint>
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/MIDISoundBank.h"
#include "Hardware/Audio/SID.h"

namespace GUI::MIDI {

struct ActiveSIDVoice {
    int index = 0;
    int note = -1;
    int channel = -1;
    int program = 0;
    MIDITrackRole role = MIDITrackRole::Harmony;
    double startTimeSec = 0.0;
    bool active = false;
    bool released = false;
    double releaseTimerSec = 0.0;
    double maxReleaseTimeSec = 0.05;
    int priorityScore = 0;
    std::uint32_t noteId = 0;
    std::uint8_t waveMask = SID_WAVE_PULSE;
    std::uint16_t pulseWidth = 0x0800;
    int pwmDir = 1;
    std::uint16_t features = 0;
    bool isModulator = false;
    int lfoCounter = 0;
    int kickSlideStep = 0;
    std::uint16_t kickBaseFreqHi = 0;
};

class SIDLiveSynthesizer {
   public:
    SIDLiveSynthesizer() = default;

    void ResetRegCache();
    void WriteReg(Hardware::SID& sid, std::uint8_t reg, std::uint8_t val, bool force = false);

    [[nodiscard]] std::uint16_t NoteToSIDFreq(double midiNote) const;
    [[nodiscard]] std::uint16_t FreqHzToCutoffDAC(double hz, Hardware::SIDModel model) const;

    [[nodiscard]] int ComputePriority(int note, int channel, MIDITrackRole role, int mainChannel,
                                      int bassChannel) const;

    int AllocateVoice(int note, int channel, double nowSec, std::uint16_t features,
                      MIDITrackRole role, int mainChannel, int bassChannel);

    void TriggerVoiceNoteOn(Hardware::SID& sid, int voiceIdx, int channel, int program, int note,
                            int velocity, double nowSec, MIDITrackRole role, int mainChannel,
                            int bassChannel, std::uint8_t customWaveOverride);

    void TriggerVoiceNoteOff(Hardware::SID& sid, int voiceIdx);
    void TriggerVoiceNoteOffByNoteId(Hardware::SID& sid, std::uint32_t noteId);

    void StepModulationEffects(Hardware::SID& sid, double stepSec, int mainChannel);

    void AllNotesOff(Hardware::SID& sid);
    void ReleaseAllActiveVoices();
    void Reset();

    [[nodiscard]] bool AnyVoiceActive() const;
    [[nodiscard]] const std::array<ActiveSIDVoice, 3>& GetVoices() const { return voices_; }
    [[nodiscard]] std::array<ActiveSIDVoice, 3>& GetVoices() { return voices_; }
    [[nodiscard]] ActiveSIDVoice& GetVoice(std::size_t idx) { return voices_.at(idx); }
    [[nodiscard]] const ActiveSIDVoice& GetVoice(std::size_t idx) const { return voices_.at(idx); }

    [[nodiscard]] bool IsNTSC() const { return isNTSC_; }
    void SetNTSC(bool ntsc) { isNTSC_ = ntsc; }

    [[nodiscard]] int GetRegCache(std::size_t reg) const { return sidRegCache_.at(reg); }

   private:
    std::array<ActiveSIDVoice, 3> voices_{{{0}, {1}, {2}}};
    std::array<int, 25> sidRegCache_{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
    bool isNTSC_ = true;
};

}  // namespace GUI::MIDI
