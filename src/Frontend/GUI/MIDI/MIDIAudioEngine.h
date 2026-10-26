#pragma once

#include <array>
#include <cstdint>
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/MIDISequencerTransport.h"
#include "Frontend/GUI/MIDI/SIDLiveSynthesizer.h"

namespace GUI::MIDI {

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

    [[nodiscard]] bool IsPlaying() const { return transport_.IsPlaying(); }
    [[nodiscard]] bool IsActive() const { return transport_.IsPlaying() || auditionActive_ || synth_.AnyVoiceActive(); }

    [[nodiscard]] double GetPlayheadBeats() const { return transport_.GetPlayheadBeats(); }
    void SetPlayheadBeats(double beats);

    [[nodiscard]] bool IsLoopEnabled() const { return transport_.IsLoopEnabled(); }
    void SetLoopEnabled(bool loop) { transport_.SetLoopEnabled(loop); }

    [[nodiscard]] double GetLoopStartBeats() const { return transport_.GetLoopStartBeats(); }
    [[nodiscard]] double GetLoopEndBeats() const { return transport_.GetLoopEndBeats(); }
    void SetLoopRegion(double startBeats, double endBeats) { transport_.SetLoopRegion(startBeats, endBeats); }

    [[nodiscard]] bool IsNTSC() const { return synth_.IsNTSC(); }
    void SetNTSC(bool ntsc) { synth_.SetNTSC(ntsc); }

    [[nodiscard]] const std::array<ActiveSIDVoice, 3>& GetVoices() const { return synth_.GetVoices(); }

    [[nodiscard]] const MIDISequencerTransport& GetTransport() const { return transport_; }
    [[nodiscard]] MIDISequencerTransport& GetTransport() { return transport_; }

    [[nodiscard]] const SIDLiveSynthesizer& GetSynthesizer() const { return synth_; }
    [[nodiscard]] SIDLiveSynthesizer& GetSynthesizer() { return synth_; }

   private:
    MIDISequencerTransport transport_;
    SIDLiveSynthesizer synth_;

    double effectAccumulatorSec_ = 0.0;

    bool auditionActive_ = false;
    int auditionVoiceIndex_ = 0;
    int auditionNote_ = -1;
    int auditionChannel_ = 0;
    double auditionElapsedSec_ = 0.0;
    bool auditionReleased_ = false;

    void EnsureSIDReady(Control::AppState& state);
};

}  // namespace GUI::MIDI
