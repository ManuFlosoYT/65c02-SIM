#include "Frontend/GUI/MIDI/MIDIAudioEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Frontend/GUI/MIDI/MIDIVoiceAllocator.h"

namespace GUI::MIDI {

void MIDIAudioEngine::EnsureSIDReady(Control::AppState& state) {
    state.emulator.Pause();
    Hardware::SID& sid = state.emulator.GetSID();
    if (!sid.IsSoundEnabled()) {
        sid.EnableSound(true);
    }
    sid.SetEmulationPaused(false);
    constexpr double CLOCK_NTSC_HZ = 1000000.0;
    constexpr double CLOCK_PAL_HZ = 985248.0;
    const double sidClock = synth_.IsNTSC() ? CLOCK_NTSC_HZ : CLOCK_PAL_HZ;
    sid.SetClockRate(sidClock);
    sid.SetCpuClockRate(sidClock);
    if (synth_.GetRegCache(0x18) == -1) {
        synth_.WriteReg(sid, 0x18, 0x0F, true);
    }
}

void MIDIAudioEngine::TriggerAuditionNote(Control::AppState& state, int channel, int program, int note, int velocity,
                                         std::uint8_t customWaveOverride, MIDITrackRole role) {
    state.emulator.Pause();
    synth_.ResetRegCache();
    EnsureSIDReady(state);
    Hardware::SID& sid = state.emulator.GetSID();

    const SIDPatchParams params =
        MIDISoundBank::GetInstrumentParams(program, channel, note, velocity, sid.GetModel(), customWaveOverride);

    int voiceIdx = 0;
    if (auditionActive_ && auditionVoiceIndex_ >= 0 && auditionVoiceIndex_ < 3) {
        voiceIdx = auditionVoiceIndex_;
    } else if (synth_.AnyVoiceActive()) {
        voiceIdx = synth_.AllocateVoice(note, channel, transport_.GetPlayheadSeconds(), params.features, role, channel, -1);
        if (voiceIdx < 0) {
            voiceIdx = 0;
        }
    }

    synth_.TriggerVoiceNoteOn(sid, voiceIdx, channel, program, note, velocity, transport_.GetPlayheadSeconds(), role,
                              channel, -1, customWaveOverride);

    auditionActive_ = true;
    auditionVoiceIndex_ = voiceIdx;
    auditionNote_ = note;
    auditionChannel_ = channel;
    auditionElapsedSec_ = 0.0;
    auditionReleased_ = false;

    sid.Clock(256U);
}

void MIDIAudioEngine::ReleaseAuditionNote(Control::AppState& state) {
    if (!auditionActive_) {
        return;
    }
    auditionReleased_ = true;
    if (auditionElapsedSec_ >= 0.12) {
        Hardware::SID& sid = state.emulator.GetSID();
        synth_.TriggerVoiceNoteOff(sid, auditionVoiceIndex_);
        auditionActive_ = false;
        auditionReleased_ = false;
    }
}

void MIDIAudioEngine::Play(Control::AppState& state, const MIDIProject& project) {
    state.emulator.Pause();
    synth_.ResetRegCache();
    EnsureSIDReady(state);
    transport_.Play(project);
}

void MIDIAudioEngine::Pause(Control::AppState& state) {
    transport_.Pause();
    Hardware::SID& sid = state.emulator.GetSID();
    AllNotesOff(sid);
    sid.SetEmulationPaused(state.emulator.IsPaused());
    sid.SetCpuClockRate(static_cast<double>(state.emulator.GetTargetIPS()));
}

void MIDIAudioEngine::Stop(Control::AppState& state) {
    transport_.Stop();
    auditionActive_ = false;
    auditionReleased_ = false;
    synth_.ResetRegCache();
    Hardware::SID& sid = state.emulator.GetSID();
    AllNotesOff(sid);
    sid.SetEmulationPaused(state.emulator.IsPaused());
    sid.SetCpuClockRate(static_cast<double>(state.emulator.GetTargetIPS()));
}

void MIDIAudioEngine::Reset(Control::AppState& state) {
    synth_.ResetRegCache();
    Stop(state);
}

void MIDIAudioEngine::Reset() {
    synth_.Reset();
    transport_.Reset();
}

void MIDIAudioEngine::AllNotesOff(Hardware::SID& sid) {
    synth_.AllNotesOff(sid);
    transport_.ClearSequenceNotes();
}

void MIDIAudioEngine::SetPlayheadBeats(double beats) {
    transport_.SetPlayheadBeats(beats, synth_);
}

void MIDIAudioEngine::Update(Control::AppState& state, const MIDIProject& project, float deltaSeconds) {
    if (!IsActive()) {
        return;
    }

    EnsureSIDReady(state);
    Hardware::SID& sid = state.emulator.GetSID();

    const double clampedDelta = static_cast<double>(std::clamp(deltaSeconds, 0.001F, 0.050F));
    constexpr double SLICE_SEC = 0.001;
    const std::uint32_t cyclesPerMs = synth_.IsNTSC() ? 1000U : 985U;

    int mainChannel = -1;
    int bassChannel = -1;
    MIDIVoiceAllocator::DetectProjectChannels(project, mainChannel, bassChannel);

    double remainingSec = clampedDelta;
    while (remainingSec >= (SLICE_SEC * 0.5)) {
        const double stepSec = std::min(remainingSec, SLICE_SEC);
        remainingSec -= stepSec;

        if (transport_.IsPlaying()) {
            transport_.AdvanceStep(sid, project, stepSec, synth_);
        }

        if (auditionActive_) {
            auditionElapsedSec_ += stepSec;
            const bool isDrum = (auditionChannel_ == 9);
            if ((isDrum && auditionElapsedSec_ >= 0.15) || (auditionReleased_ && auditionElapsedSec_ >= 0.12)) {
                synth_.TriggerVoiceNoteOff(sid, auditionVoiceIndex_);
                auditionActive_ = false;
                auditionReleased_ = false;
            }
        }

        effectAccumulatorSec_ += stepSec;
        while (effectAccumulatorSec_ >= 0.010) {
            effectAccumulatorSec_ -= 0.010;
            synth_.StepModulationEffects(sid, 0.010, mainChannel);
        }

        sid.Clock(cyclesPerMs);
    }
}

}  // namespace GUI::MIDI
