#pragma once

#include <cstdint>
#include <vector>
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/SIDLiveSynthesizer.h"
#include "Hardware/Audio/SID.h"

namespace GUI::MIDI {

class MIDISequencerTransport {
   public:
    MIDISequencerTransport() = default;

    void Play(const MIDIProject& project);
    void Pause();
    void Stop();
    void Reset();

    [[nodiscard]] bool IsPlaying() const { return playing_; }

    [[nodiscard]] double GetPlayheadBeats() const { return playheadBeats_; }
    void SetPlayheadBeats(double beats, SIDLiveSynthesizer& synth);

    [[nodiscard]] double GetPlayheadSeconds() const { return playheadSeconds_; }
    void SetPlayheadSeconds(double seconds) { playheadSeconds_ = seconds; }

    [[nodiscard]] bool IsLoopEnabled() const { return loopEnabled_; }
    void SetLoopEnabled(bool loop) { loopEnabled_ = loop; }

    [[nodiscard]] double GetLoopStartBeats() const { return loopStartBeats_; }
    [[nodiscard]] double GetLoopEndBeats() const { return loopEndBeats_; }
    void SetLoopRegion(double startBeats, double endBeats);

    void ClearSequenceNotes() { activeSequenceNotes_.clear(); }

    void StepSequencerSlice(Hardware::SID& sid, const MIDIProject& project, double sliceStartSec,
                            double sliceEndSec, SIDLiveSynthesizer& synth);

    void AdvanceStep(Hardware::SID& sid, const MIDIProject& project, double stepSec,
                     SIDLiveSynthesizer& synth);

   private:
    bool playing_ = false;
    double playheadBeats_ = 0.0;
    double playheadSeconds_ = 0.0;

    bool loopEnabled_ = true;
    double loopStartBeats_ = 0.0;
    double loopEndBeats_ = 16.0;

    std::vector<std::uint32_t> activeSequenceNotes_;
};

}  // namespace GUI::MIDI
