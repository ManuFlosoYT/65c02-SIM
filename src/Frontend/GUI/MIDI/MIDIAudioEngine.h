#pragma once

#include <array>
#include <cstdint>
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/MIDISoundBank.h"

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

class MIDIAudioEngine {
   public:
    MIDIAudioEngine() = default;

    void Update(Control::AppState& state, const MIDIProject& project, float deltaSeconds);

    void TriggerAuditionNote(Control::AppState& state, int channel, int program, int note, int velocity,
                             std::uint8_t customWaveOverride = 0,
                             MIDITrackRole role = MIDITrackRole::Melody);
    void ReleaseAuditionNote(Control::AppState& state);

    void Play(Control::AppState& state, const MIDIProject& project);
    void Pause(Control::AppState& state);
    void Stop(Control::AppState& state);
    void Reset(Control::AppState& state);
    void Reset();
    void AllNotesOff(Hardware::SID& sid);

    [[nodiscard]] bool IsPlaying() const { return playing; }
    [[nodiscard]] bool IsActive() const { return playing || auditionActive || AnyVoiceActive(); }

    [[nodiscard]] double GetPlayheadBeats() const { return playheadBeats; }
    void SetPlayheadBeats(double beats);

    [[nodiscard]] bool IsLoopEnabled() const { return loopEnabled; }
    void SetLoopEnabled(bool loop) { loopEnabled = loop; }

    [[nodiscard]] double GetLoopStartBeats() const { return loopStartBeats; }
    [[nodiscard]] double GetLoopEndBeats() const { return loopEndBeats; }
    void SetLoopRegion(double startBeats, double endBeats);

    [[nodiscard]] bool IsNTSC() const { return isNTSC; }
    void SetNTSC(bool ntsc) { isNTSC = ntsc; }

    [[nodiscard]] const std::array<ActiveSIDVoice, 3>& GetVoices() const { return voices; }

   private:
    std::array<ActiveSIDVoice, 3> voices{{{0}, {1}, {2}}};
    std::array<int, 25> sidRegCache{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

    bool playing = false;
    double playheadBeats = 0.0;
    double playheadSeconds = 0.0;
    double effectAccumulatorSec = 0.0;

    bool loopEnabled = true;
    double loopStartBeats = 0.0;
    double loopEndBeats = 16.0;
    bool isNTSC = true;

    bool auditionActive = false;
    int auditionVoiceIndex = 0;
    int auditionNote = -1;
    int auditionChannel = 0;
    double auditionElapsedSec = 0.0;
    bool auditionReleased = false;

    std::vector<std::uint32_t> activeSequenceNotes;

    [[nodiscard]] bool AnyVoiceActive() const;
    void EnsureSIDReady(Control::AppState& state);
    void WriteReg(Hardware::SID& sid, std::uint8_t reg, std::uint8_t val, bool force = false);
    [[nodiscard]] std::uint16_t NoteToSIDFreq(double midiNote) const;
    [[nodiscard]] std::uint16_t FreqHzToCutoffDAC(double hz, Hardware::SIDModel model) const;
    [[nodiscard]] int ComputePriority(int note, int channel, MIDITrackRole role, int mainChannel, int bassChannel) const;
    int AllocateVoice(int note, int channel, double nowSec, std::uint16_t features, MIDITrackRole role,
                      int mainChannel, int bassChannel);
    void TriggerVoiceNoteOn(Hardware::SID& sid, int voiceIdx, int channel, int program, int note, int velocity,
                            double nowSec, MIDITrackRole role, int mainChannel, int bassChannel,
                            std::uint8_t customWaveOverride);
    void TriggerVoiceNoteOff(Hardware::SID& sid, int voiceIdx);
    void StepModulationEffects(Hardware::SID& sid, double stepSec, int mainChannel);
    void StepSequencerSlice(Hardware::SID& sid, const MIDIProject& project, double sliceStartSec, double sliceEndSec);
};

}  // namespace GUI::MIDI

