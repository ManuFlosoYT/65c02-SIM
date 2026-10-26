#include "Frontend/GUI/MIDI/MIDISequencerTransport.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include "Frontend/GUI/MIDI/MIDIVoiceAllocator.h"

namespace GUI::MIDI {

namespace {

struct PendingNoteOn {
    std::uint32_t noteId = 0;
    int channel = 0;
    int program = 0;
    int pitch = 60;
    int velocity = 100;
    double startSec = 0.0;
    MIDITrackRole role = MIDITrackRole::Harmony;
    std::uint8_t customWaveOverride = 0;
    std::uint16_t features = 0;
    int priority = 0;
};

}  // namespace

void MIDISequencerTransport::Play(const MIDIProject& project) {
    playheadSeconds_ = project.BeatsToSeconds(playheadBeats_);
    playing_ = true;
}

void MIDISequencerTransport::Pause() {
    playing_ = false;
}

void MIDISequencerTransport::Stop() {
    playing_ = false;
    playheadBeats_ = loopEnabled_ ? loopStartBeats_ : 0.0;
    playheadSeconds_ = 0.0;
    activeSequenceNotes_.clear();
}

void MIDISequencerTransport::Reset() {
    playing_ = false;
    playheadBeats_ = 0.0;
    playheadSeconds_ = 0.0;
    activeSequenceNotes_.clear();
}

void MIDISequencerTransport::SetPlayheadBeats(double beats, SIDLiveSynthesizer& synth) {
    playheadBeats_ = std::max(0.0, beats);
    activeSequenceNotes_.clear();
    synth.ReleaseAllActiveVoices();
}

void MIDISequencerTransport::SetLoopRegion(double startBeats, double endBeats) {
    loopStartBeats_ = std::max(0.0, startBeats);
    loopEndBeats_ = std::max(loopStartBeats_ + 0.25, endBeats);
}

void MIDISequencerTransport::StepSequencerSlice(Hardware::SID& sid, const MIDIProject& project,
                                                double sliceStartSec, double sliceEndSec,
                                                SIDLiveSynthesizer& synth) {
    int mainChannel = -1;
    int bassChannel = -1;
    MIDIVoiceAllocator::DetectProjectChannels(project, mainChannel, bassChannel);

    bool anySolo = false;
    for (const auto& track : project.tracks) {
        if (track.solo) {
            anySolo = true;
            break;
        }
    }

    std::vector<PendingNoteOn> pendingNoteOns;

    for (const auto& track : project.tracks) {
        const bool trackAudible = anySolo ? track.solo : !track.muted;

        for (const auto& note : track.notes) {
            const double noteStartSec = project.BeatsToSeconds(note.startBeat);
            double noteEndSec = project.BeatsToSeconds(note.startBeat + note.durationBeats);

            const SIDPatchParams params = MIDISoundBank::GetInstrumentParams(
                track.program, track.channel, note.pitch, note.velocity, sid.GetModel(), track.customWaveOverride);

            if (track.channel == 9 || track.role == MIDITrackRole::Percussion) {
                const double drumMaxEndSec = noteStartSec + static_cast<double>(params.maxDrumLengthSec);
                noteEndSec = std::min(noteEndSec, drumMaxEndSec);
            }

            const auto activeIt = std::find(activeSequenceNotes_.begin(), activeSequenceNotes_.end(), note.id);
            if (activeIt != activeSequenceNotes_.end()) {
                if (!trackAudible || sliceEndSec >= noteEndSec) {
                    activeSequenceNotes_.erase(activeIt);
                    synth.TriggerVoiceNoteOffByNoteId(sid, note.id);
                }
                continue;
            }

            if (!trackAudible) {
                continue;
            }

            if (noteStartSec >= (sliceStartSec - 1e-6) && noteStartSec < (sliceEndSec - 1e-6) &&
                sliceStartSec < noteEndSec) {
                const MIDITrackRole effectiveRole =
                    MIDIVoiceAllocator::ResolveTrackRole(track.role, track.channel, mainChannel, bassChannel);
                PendingNoteOn item;
                item.noteId = note.id;
                item.channel = track.channel;
                item.program = track.program;
                item.pitch = note.pitch;
                item.velocity = note.velocity;
                item.startSec = noteStartSec;
                item.role = effectiveRole;
                item.customWaveOverride = track.customWaveOverride;
                item.features = params.features;
                item.priority =
                    MIDIVoiceAllocator::ComputeNotePriority(note.pitch, effectiveRole, false, track.channel);
                pendingNoteOns.push_back(item);
            }
        }
    }

    std::sort(pendingNoteOns.begin(), pendingNoteOns.end(), [](const PendingNoteOn& a, const PendingNoteOn& b) {
        if (std::abs(a.startSec - b.startSec) > 1e-6) {
            return a.startSec < b.startSec;
        }
        return a.priority > b.priority;
    });

    for (const auto& item : pendingNoteOns) {
        const int voiceIdx = synth.AllocateVoice(item.pitch, item.channel, item.startSec, item.features, item.role,
                                                 mainChannel, bassChannel);
        if (voiceIdx >= 0) {
            synth.TriggerVoiceNoteOn(sid, voiceIdx, item.channel, item.program, item.pitch, item.velocity,
                                     item.startSec, item.role, mainChannel, bassChannel, item.customWaveOverride);
            synth.GetVoice(static_cast<std::size_t>(voiceIdx)).noteId = item.noteId;
            activeSequenceNotes_.push_back(item.noteId);
        }
    }
}

void MIDISequencerTransport::AdvanceStep(Hardware::SID& sid, const MIDIProject& project, double stepSec,
                                        SIDLiveSynthesizer& synth) {
    if (!playing_) {
        return;
    }

    playheadSeconds_ = project.BeatsToSeconds(playheadBeats_);
    const double sliceStartSec = playheadSeconds_;
    const double sliceEndSec = sliceStartSec + stepSec;

    StepSequencerSlice(sid, project, sliceStartSec, sliceEndSec, synth);

    playheadSeconds_ = sliceEndSec;
    playheadBeats_ = project.SecondsToBeats(playheadSeconds_);

    if (loopEnabled_ && loopEndBeats_ > loopStartBeats_ && playheadBeats_ >= loopEndBeats_) {
        synth.AllNotesOff(sid);
        activeSequenceNotes_.clear();
        playheadBeats_ = loopStartBeats_;
        playheadSeconds_ = project.BeatsToSeconds(playheadBeats_);
    } else if (!loopEnabled_) {
        const double totalBeats = std::max(project.GetTotalDurationBeats(), loopEndBeats_);
        if (playheadBeats_ >= totalBeats && !synth.AnyVoiceActive()) {
            playing_ = false;
            playheadBeats_ = 0.0;
            playheadSeconds_ = 0.0;
        }
    }
}

}  // namespace GUI::MIDI
