#include "Frontend/GUI/MIDI/MIDIAudioEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace GUI::MIDI {

namespace {

constexpr std::array<double, 16> SID_RELEASE_TIMES = {
    0.006, 0.024, 0.048, 0.072, 0.114, 0.168, 0.204, 0.240,
    0.300, 0.750, 1.500, 2.400, 3.000, 9.000, 15.000, 24.000
};

constexpr std::uint8_t REG_FREQ_LO = 0x00;
constexpr std::uint8_t REG_FREQ_HI = 0x01;
constexpr std::uint8_t REG_PW_LO = 0x02;
constexpr std::uint8_t REG_PW_HI = 0x03;
constexpr std::uint8_t REG_CTRL = 0x04;
constexpr std::uint8_t REG_AD = 0x05;
constexpr std::uint8_t REG_SR = 0x06;

constexpr std::uint8_t REG_CUTOFF_LO = 0x15;
constexpr std::uint8_t REG_CUTOFF_HI = 0x16;
constexpr std::uint8_t REG_RESON_FILT = 0x17;
constexpr std::uint8_t REG_MODE_VOL = 0x18;

constexpr std::uint8_t FILT_LP = 0x10;
constexpr std::uint8_t FILT_BP = 0x20;
constexpr std::uint8_t FILT_HP = 0x40;
constexpr std::uint8_t FILT_OFF3 = 0x80;

constexpr double CLOCK_NTSC_HZ = 1000000.0;
constexpr double CLOCK_PAL_HZ = 985248.0;

constexpr bool HasFeature(std::uint16_t features, SIDFeatureFlags flag) {
    return (features & static_cast<std::uint16_t>(flag)) != 0;
}

MIDITrackRole ResolveRole(MIDITrackRole role, int channel, int mainChannel, int bassChannel) {
    if (channel == 9 || role == MIDITrackRole::Percussion) {
        return MIDITrackRole::Percussion;
    }
    if (role != MIDITrackRole::Auto) {
        return role;
    }
    if (mainChannel >= 0 && channel == mainChannel) {
        return MIDITrackRole::Melody;
    }
    if (bassChannel >= 0 && channel == bassChannel) {
        return MIDITrackRole::Bass;
    }
    return MIDITrackRole::Harmony;
}

void DetectProjectChannels(const MIDIProject& project, int& outMainChannel, int& outBassChannel) {
    outMainChannel = -1;
    outBassChannel = -1;

    for (const auto& track : project.tracks) {
        if (track.channel == 9 || track.role == MIDITrackRole::Percussion) {
            continue;
        }
        if ((track.isMainTrack || track.role == MIDITrackRole::Melody) && outMainChannel == -1) {
            outMainChannel = track.channel;
        }
        if (track.role == MIDITrackRole::Bass && outBassChannel == -1) {
            outBassChannel = track.channel;
        }
    }

    if (outMainChannel != -1 && outBassChannel != -1) {
        return;
    }

    double highestAvg = -1.0;
    double lowestAvg = 1000.0;
    int highestCh = -1;
    int lowestCh = -1;

    for (const auto& track : project.tracks) {
        if (track.channel == 9 || track.role == MIDITrackRole::Percussion || track.notes.empty()) {
            continue;
        }
        double sum = 0.0;
        for (const auto& note : track.notes) {
            sum += static_cast<double>(note.pitch);
        }
        const double avg = sum / static_cast<double>(track.notes.size());
        if (avg > highestAvg) {
            highestAvg = avg;
            highestCh = track.channel;
        }
        if (avg < lowestAvg) {
            lowestAvg = avg;
            lowestCh = track.channel;
        }
    }

    if (outMainChannel == -1) {
        outMainChannel = highestCh;
    }
    if (outBassChannel == -1 && lowestCh != outMainChannel) {
        outBassChannel = lowestCh;
    }
}

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

bool MIDIAudioEngine::AnyVoiceActive() const {
    return voices[0].active || voices[1].active || voices[2].active;
}

void MIDIAudioEngine::EnsureSIDReady(Control::AppState& state) {
    state.emulator.Pause();
    Hardware::SID& sid = state.emulator.GetSID();
    if (!sid.IsSoundEnabled()) {
        sid.EnableSound(true);
    }
    sid.SetEmulationPaused(false);
    const double sidClock = isNTSC ? CLOCK_NTSC_HZ : CLOCK_PAL_HZ;
    sid.SetClockRate(sidClock);
    sid.SetCpuClockRate(sidClock);
    if (sidRegCache.at(REG_MODE_VOL) == -1) {
        WriteReg(sid, REG_MODE_VOL, 0x0F, true);
    }
}

void MIDIAudioEngine::WriteReg(Hardware::SID& sid, std::uint8_t reg, std::uint8_t val, bool force) {
    reg &= 0x1FU;
    if (force || reg >= 25U || sidRegCache.at(reg) != static_cast<int>(val)) {
        if (reg < 25U) {
            sidRegCache.at(reg) = static_cast<int>(val);
        }
        sid.Write(reg, val);
    }
}

std::uint16_t MIDIAudioEngine::NoteToSIDFreq(double midiNote) const {
    const double sidClock = isNTSC ? CLOCK_NTSC_HZ : CLOCK_PAL_HZ;
    const double hz = 440.0 * std::pow(2.0, (midiNote - 69.0) / 12.0);
    const double freq = std::round((hz * 16777216.0) / sidClock);
    const double clamped = std::clamp(freq, 0.0, 65535.0);
    return static_cast<std::uint16_t>(clamped);
}

std::uint16_t MIDIAudioEngine::FreqHzToCutoffDAC(double hz, Hardware::SIDModel model) const {
    if (!isNTSC && hz < -1.0) {
        return 0;
    }
    const double safeHz = std::max(0.0, hz);
    int dac = 0;
    if (model == Hardware::SIDModel::MOS6581) {
        dac = static_cast<int>(2047.0 * std::pow(safeHz / 12500.0, 0.4));
    } else {
        dac = static_cast<int>((safeHz / 12500.0) * 2047.0);
    }
    return static_cast<std::uint16_t>(std::clamp(dac, 0, 2047));
}

int MIDIAudioEngine::ComputePriority(int note, int channel, MIDITrackRole role, int mainChannel, int bassChannel) const {
    if (!isNTSC && note < -128) {
        return 0;
    }
    const MIDITrackRole effectiveRole = ResolveRole(role, channel, mainChannel, bassChannel);
    int score = 1000;
    if (effectiveRole == MIDITrackRole::Melody) {
        score = 8000;
    } else if (effectiveRole == MIDITrackRole::Bass) {
        score = 5000;
    } else if (effectiveRole == MIDITrackRole::Percussion) {
        score = 3000;
    }

    if (note > 80 || note < 40) {
        score += 100;
    }
    return score + note;
}

int MIDIAudioEngine::AllocateVoice(int note, int channel, double nowSec, std::uint16_t features, MIDITrackRole role,
                                   int mainChannel, int bassChannel) {
    const MIDITrackRole effectiveRole = ResolveRole(role, channel, mainChannel, bassChannel);
    const bool isPerc = (channel == 9 || effectiveRole == MIDITrackRole::Percussion);

    if (isPerc) {
        int activePerc = 0;
        bool melOrBassActive = false;
        for (const auto& v : voices) {
            if (v.active && !v.released && (v.channel == 9 || v.role == MIDITrackRole::Percussion)) {
                ++activePerc;
            }
            if (v.active && (v.role == MIDITrackRole::Melody || v.role == MIDITrackRole::Bass)) {
                melOrBassActive = true;
            }
        }
        if (activePerc >= 1 && melOrBassActive) {
            return -1;
        }
    }

    int bestInactive = -1;
    for (int i = 0; i < 3; ++i) {
        const auto& v = voices.at(static_cast<std::size_t>(i));
        if (!v.active) {
            if (v.channel == channel) {
                bestInactive = i;
                break;
            }
            if (bestInactive == -1) {
                bestInactive = i;
            }
        }
    }

    int chosenIdx = bestInactive;
    if (chosenIdx == -1) {
        const int incomingPrio = ComputePriority(note, channel, effectiveRole, mainChannel, bassChannel);
        int bestScore = std::numeric_limits<int>::max();

        for (int i = 0; i < 3; ++i) {
            const auto& v = voices.at(static_cast<std::size_t>(i));
            int effScore = 0;
            if (v.released) {
                if ((v.role == MIDITrackRole::Melody || v.role == MIDITrackRole::Bass) && v.releaseTimerSec < 0.120) {
                    if (incomingPrio < 5000) {
                        continue;
                    }
                }
                effScore = v.priorityScore - 30000;
            } else {
                effScore = v.priorityScore;
                if ((nowSec - v.startTimeSec) < 0.05) {
                    effScore += 5000;
                }
                if (v.channel == channel) {
                    effScore -= 2000;
                }
            }

            if (v.released || incomingPrio > v.priorityScore) {
                if (effScore < bestScore) {
                    bestScore = effScore;
                    chosenIdx = i;
                }
            }
        }

        if (chosenIdx == -1) {
            return -1;
        }
    }

    if (HasFeature(features, SIDFeatureFlags::HardSync) || HasFeature(features, SIDFeatureFlags::RingMod)) {
        const auto modIdx = static_cast<std::size_t>((chosenIdx + 2) % 3);
        voices.at(modIdx).isModulator = true;
    }

    return chosenIdx;
}

void MIDIAudioEngine::TriggerVoiceNoteOn(Hardware::SID& sid, int voiceIdx, int channel, int program, int note,
                                         int velocity, double nowSec, MIDITrackRole role, int mainChannel,
                                         int bassChannel, std::uint8_t customWaveOverride) {
    if (voiceIdx < 0 || voiceIdx >= 3) {
        return;
    }

    const SIDPatchParams params =
        MIDISoundBank::GetInstrumentParams(program, channel, note, velocity, sid.GetModel(), customWaveOverride);
    const MIDITrackRole effectiveRole = ResolveRole(role, channel, mainChannel, bassChannel);

    ActiveSIDVoice& v = voices.at(static_cast<std::size_t>(voiceIdx));
    v.index = voiceIdx;
    v.note = note;
    v.channel = channel;
    v.program = program;
    v.role = effectiveRole;
    v.startTimeSec = nowSec;
    v.active = true;
    v.released = false;
    v.releaseTimerSec = 0.0;
    v.noteId = 0;
    v.maxReleaseTimeSec = SID_RELEASE_TIMES.at(static_cast<std::size_t>(params.effectiveSustainRelease & 0x0FU));
    v.priorityScore = ComputePriority(note, channel, effectiveRole, mainChannel, bassChannel);
    v.waveMask = params.waveMask;
    v.pulseWidth = params.pulseWidth;
    v.pwmDir = 1;
    v.features = params.features;
    v.lfoCounter = 0;
    v.kickSlideStep = 0;
    v.kickBaseFreqHi = 0;

    const auto base = static_cast<std::uint8_t>(voiceIdx * 7);

    WriteReg(sid, static_cast<std::uint8_t>(base + REG_CTRL),
             static_cast<std::uint8_t>(params.waveMask & ~SID_WAVE_GATE), true);
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_AD), params.attackDecay);
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_SR), params.effectiveSustainRelease);

    if ((params.waveMask & (SID_WAVE_PULSE | SID_WAVE_TEST)) != 0) {
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_PW_LO), static_cast<std::uint8_t>(params.pulseWidth & 0xFFU));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_PW_HI),
                 static_cast<std::uint8_t>((params.pulseWidth >> 8U) & 0x0FU));
    }

    const bool hasFilt = HasFeature(params.features, SIDFeatureFlags::BassFilter) ||
                         HasFeature(params.features, SIDFeatureFlags::VelocityFilter) ||
                         HasFeature(params.features, SIDFeatureFlags::FilterHP) ||
                         HasFeature(params.features, SIDFeatureFlags::FilterBP);

    if (hasFilt) {
        const bool isMelody = (effectiveRole == MIDITrackRole::Melody);
        bool melodyActive = false;
        for (const auto& vo : voices) {
            if (vo.active && vo.role == MIDITrackRole::Melody) {
                melodyActive = true;
                break;
            }
        }

        if (isMelody || !melodyActive) {
            const auto filtRoute = static_cast<std::uint8_t>(1U << static_cast<unsigned>(voiceIdx));
            double targetHz = 1000.0;
            if (HasFeature(params.features, SIDFeatureFlags::VelocityFilter)) {
                targetHz = 200.0 + ((static_cast<double>(velocity) / 127.0) * 8000.0);
            } else if (HasFeature(params.features, SIDFeatureFlags::BassFilter)) {
                targetHz = 1500.0;
            }

            const std::uint16_t dacVal = FreqHzToCutoffDAC(targetHz, sid.GetModel());
            WriteReg(sid, REG_CUTOFF_LO, static_cast<std::uint8_t>(dacVal & 0x07U));
            WriteReg(sid, REG_CUTOFF_HI, static_cast<std::uint8_t>((dacVal >> 3U) & 0xFFU));

            std::uint8_t res = 0x40;
            if (sid.GetModel() == Hardware::SIDModel::MOS6581 && dacVal > 1500U) {
                res = 0x10;
            }
            WriteReg(sid, REG_RESON_FILT, static_cast<std::uint8_t>(res | filtRoute));

            int currVol = sidRegCache.at(REG_MODE_VOL);
            if (currVol == -1) {
                currVol = 0x0F;
            }
            std::uint8_t fmode = FILT_LP;
            if (HasFeature(params.features, SIDFeatureFlags::FilterHP)) {
                fmode = FILT_HP;
            } else if (HasFeature(params.features, SIDFeatureFlags::FilterBP)) {
                fmode = FILT_BP;
            }
            if (voices.at(2).isModulator) {
                fmode |= FILT_OFF3;
            }
            const auto modeVol = static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(currVol) & ~(FILT_LP | FILT_BP | FILT_HP | FILT_OFF3)) | fmode);
            WriteReg(sid, REG_MODE_VOL, modeVol);
        }
    } else if (sidRegCache.at(REG_RESON_FILT) != -1) {
        const auto voiceBit = static_cast<std::uint8_t>(1U << static_cast<unsigned>(voiceIdx));
        const auto currResFilt = static_cast<std::uint8_t>(sidRegCache.at(REG_RESON_FILT));
        if ((currResFilt & voiceBit) != 0) {
            WriteReg(sid, REG_RESON_FILT, static_cast<std::uint8_t>(currResFilt & ~voiceBit));
        }
    }

    auto ctrlVal = static_cast<std::uint8_t>(params.waveMask | SID_WAVE_GATE);
    if (HasFeature(params.features, SIDFeatureFlags::HardSync)) {
        ctrlVal |= SID_WAVE_SYNC;
    }
    if (HasFeature(params.features, SIDFeatureFlags::RingMod)) {
        ctrlVal |= SID_WAVE_RING;
    }

    if (HasFeature(params.features, SIDFeatureFlags::KickSlide)) {
        const std::uint16_t startFreq = NoteToSIDFreq(static_cast<double>(note + 12));
        v.kickSlideStep = 1;
        v.kickBaseFreqHi = static_cast<std::uint16_t>((startFreq >> 8U) & 0xFFU);
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_LO), static_cast<std::uint8_t>(startFreq & 0xFFU));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_HI),
                 static_cast<std::uint8_t>((startFreq >> 8U) & 0xFFU));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_CTRL), ctrlVal, true);
    } else {
        const std::uint16_t freq = NoteToSIDFreq(static_cast<double>(note));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_LO), static_cast<std::uint8_t>(freq & 0xFFU));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_HI), static_cast<std::uint8_t>((freq >> 8U) & 0xFFU));
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_CTRL), ctrlVal, true);
    }
}

void MIDIAudioEngine::TriggerVoiceNoteOff(Hardware::SID& sid, int voiceIdx) {
    if (voiceIdx < 0 || voiceIdx >= 3) {
        return;
    }
    ActiveSIDVoice& v = voices.at(static_cast<std::size_t>(voiceIdx));
    if (!v.active || v.released) {
        return;
    }
    v.released = true;
    v.releaseTimerSec = 0.0;
    v.noteId = 0;

    const auto base = static_cast<std::uint8_t>(voiceIdx * 7);
    auto ctrlVal = static_cast<std::uint8_t>(v.waveMask & ~SID_WAVE_GATE);
    if (HasFeature(v.features, SIDFeatureFlags::HardSync)) {
        ctrlVal |= SID_WAVE_SYNC;
    }
    if (HasFeature(v.features, SIDFeatureFlags::RingMod)) {
        ctrlVal |= SID_WAVE_RING;
    }
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_CTRL), ctrlVal);
}

void MIDIAudioEngine::StepModulationEffects(Hardware::SID& sid, double stepSec, int mainChannel) {
    constexpr std::array<int, 3> KICK_OFFSETS = {2, 4, 8};

    for (auto& v : voices) {
        if (!v.active) {
            continue;
        }
        if (v.released) {
            v.releaseTimerSec += stepSec;
            if (v.releaseTimerSec >= v.maxReleaseTimeSec) {
                v.active = false;
                v.released = false;
                continue;
            }
        }

        const auto base = static_cast<std::uint8_t>(v.index * 7);

        if (HasFeature(v.features, SIDFeatureFlags::KickSlide) && v.kickSlideStep > 0) {
            if (v.kickSlideStep <= 3) {
                const int offset = KICK_OFFSETS.at(static_cast<std::size_t>(v.kickSlideStep - 1));
                const int nextHi = std::clamp(static_cast<int>(v.kickBaseFreqHi) - offset, 0, 255);
                WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_HI), static_cast<std::uint8_t>(nextHi));
                ++v.kickSlideStep;
                if (v.kickSlideStep > 3) {
                    v.kickSlideStep = 0;
                }
            }
        }

        ++v.lfoCounter;
        if ((v.lfoCounter % 4) != 0 || v.released) {
            continue;
        }

        if (HasFeature(v.features, SIDFeatureFlags::Vibrato)) {
            if (v.role == MIDITrackRole::Melody || v.channel == mainChannel || mainChannel < 0) {
                bool isHighest = true;
                for (const auto& other : voices) {
                    if (other.active && other.index != v.index && other.channel == v.channel && other.note > v.note) {
                        isHighest = false;
                        break;
                    }
                }
                if (isHighest) {
                    const double vibSemitones = std::sin(static_cast<double>(v.lfoCounter) * 0.5) * 0.15;
                    const std::uint16_t freq = NoteToSIDFreq(static_cast<double>(v.note) + vibSemitones);
                    WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_LO),
                             static_cast<std::uint8_t>(freq & 0xFFU));
                    WriteReg(sid, static_cast<std::uint8_t>(base + REG_FREQ_HI),
                             static_cast<std::uint8_t>((freq >> 8U) & 0xFFU));
                }
            }
        }

        if (HasFeature(v.features, SIDFeatureFlags::PWMSweep)) {
            int curPw = static_cast<int>(v.pulseWidth) + (128 * v.pwmDir);
            if (curPw > 0x0E00) {
                curPw = 0x0E00;
                v.pwmDir = -1;
            } else if (curPw < 0x0200) {
                curPw = 0x0200;
                v.pwmDir = 1;
            }
            v.pulseWidth = static_cast<std::uint16_t>(curPw);
            WriteReg(sid, static_cast<std::uint8_t>(base + REG_PW_LO),
                     static_cast<std::uint8_t>(v.pulseWidth & 0xFFU));
            WriteReg(sid, static_cast<std::uint8_t>(base + REG_PW_HI),
                     static_cast<std::uint8_t>((v.pulseWidth >> 8U) & 0x0FU));
        }
    }
}

void MIDIAudioEngine::StepSequencerSlice(Hardware::SID& sid, const MIDIProject& project, double sliceStartSec,
                                         double sliceEndSec) {
    int mainChannel = -1;
    int bassChannel = -1;
    DetectProjectChannels(project, mainChannel, bassChannel);

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

            const auto activeIt = std::find(activeSequenceNotes.begin(), activeSequenceNotes.end(), note.id);
            if (activeIt != activeSequenceNotes.end()) {
                if (!trackAudible || sliceEndSec >= noteEndSec) {
                    activeSequenceNotes.erase(activeIt);
                    for (auto& v : voices) {
                        if (v.active && !v.released && v.noteId == note.id) {
                            TriggerVoiceNoteOff(sid, v.index);
                            break;
                        }
                    }
                }
                continue;
            }

            if (!trackAudible) {
                continue;
            }

            if (noteStartSec >= (sliceStartSec - 1e-6) && noteStartSec < (sliceEndSec - 1e-6) &&
                sliceStartSec < noteEndSec) {
                const MIDITrackRole effectiveRole = ResolveRole(track.role, track.channel, mainChannel, bassChannel);
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
                item.priority = ComputePriority(note.pitch, track.channel, effectiveRole, mainChannel, bassChannel);
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
        const int voiceIdx = AllocateVoice(item.pitch, item.channel, item.startSec, item.features, item.role,
                                           mainChannel, bassChannel);
        if (voiceIdx >= 0) {
            TriggerVoiceNoteOn(sid, voiceIdx, item.channel, item.program, item.pitch, item.velocity, item.startSec,
                               item.role, mainChannel, bassChannel, item.customWaveOverride);
            voices.at(static_cast<std::size_t>(voiceIdx)).noteId = item.noteId;
            activeSequenceNotes.push_back(item.noteId);
        }
    }
}

void MIDIAudioEngine::TriggerAuditionNote(Control::AppState& state, int channel, int program, int note, int velocity,
                                          std::uint8_t customWaveOverride, MIDITrackRole role) {
    state.emulator.Pause();
    sidRegCache.fill(-1);
    EnsureSIDReady(state);
    Hardware::SID& sid = state.emulator.GetSID();

    const SIDPatchParams params =
        MIDISoundBank::GetInstrumentParams(program, channel, note, velocity, sid.GetModel(), customWaveOverride);

    int voiceIdx = 0;
    if (auditionActive && auditionVoiceIndex >= 0 && auditionVoiceIndex < 3) {
        voiceIdx = auditionVoiceIndex;
    } else if (AnyVoiceActive()) {
        voiceIdx = AllocateVoice(note, channel, playheadSeconds, params.features, role, channel, -1);
        if (voiceIdx < 0) {
            voiceIdx = 0;
        }
    }

    TriggerVoiceNoteOn(sid, voiceIdx, channel, program, note, velocity, playheadSeconds, role, channel, -1,
                       customWaveOverride);

    auditionActive = true;
    auditionVoiceIndex = voiceIdx;
    auditionNote = note;
    auditionChannel = channel;
    auditionElapsedSec = 0.0;
    auditionReleased = false;

    sid.Clock(256U);
}

void MIDIAudioEngine::ReleaseAuditionNote(Control::AppState& state) {
    if (!auditionActive) {
        return;
    }
    auditionReleased = true;
    if (auditionElapsedSec >= 0.12) {
        Hardware::SID& sid = state.emulator.GetSID();
        TriggerVoiceNoteOff(sid, auditionVoiceIndex);
        auditionActive = false;
        auditionReleased = false;
    }
}

void MIDIAudioEngine::Play(Control::AppState& state, const MIDIProject& project) {
    state.emulator.Pause();
    sidRegCache.fill(-1);
    EnsureSIDReady(state);
    playheadSeconds = project.BeatsToSeconds(playheadBeats);
    playing = true;
}

void MIDIAudioEngine::Pause(Control::AppState& state) {
    playing = false;
    Hardware::SID& sid = state.emulator.GetSID();
    AllNotesOff(sid);
    sid.SetEmulationPaused(state.emulator.IsPaused());
    sid.SetCpuClockRate(static_cast<double>(state.emulator.GetTargetIPS()));
}

void MIDIAudioEngine::Stop(Control::AppState& state) {
    playing = false;
    auditionActive = false;
    auditionReleased = false;
    playheadBeats = loopEnabled ? loopStartBeats : 0.0;
    playheadSeconds = 0.0;
    sidRegCache.fill(-1);
    Hardware::SID& sid = state.emulator.GetSID();
    AllNotesOff(sid);
    sid.SetEmulationPaused(state.emulator.IsPaused());
    sid.SetCpuClockRate(static_cast<double>(state.emulator.GetTargetIPS()));
}

void MIDIAudioEngine::Reset(Control::AppState& state) {
    sidRegCache.fill(-1);
    Stop(state);
}

void MIDIAudioEngine::Reset() {
    sidRegCache.fill(-1);
    for (auto& v : voices) {
        v.noteId = 0;
    }
}

void MIDIAudioEngine::AllNotesOff(Hardware::SID& sid) {
    for (int i = 0; i < 3; ++i) {
        auto& v = voices.at(static_cast<std::size_t>(i));
        v.active = false;
        v.released = false;
        v.releaseTimerSec = 0.0;
        v.noteId = 0;
        v.isModulator = false;
        v.kickSlideStep = 0;
        const auto base = static_cast<std::uint8_t>(i * 7);
        WriteReg(sid, static_cast<std::uint8_t>(base + REG_CTRL),
                 static_cast<std::uint8_t>(v.waveMask & ~SID_WAVE_GATE), true);
    }
    activeSequenceNotes.clear();
}

void MIDIAudioEngine::SetPlayheadBeats(double beats) {
    playheadBeats = std::max(0.0, beats);
    activeSequenceNotes.clear();
    for (auto& v : voices) {
        if (v.active && !v.released) {
            v.released = true;
            v.releaseTimerSec = 0.0;
        }
    }
}

void MIDIAudioEngine::SetLoopRegion(double startBeats, double endBeats) {
    loopStartBeats = std::max(0.0, startBeats);
    loopEndBeats = std::max(loopStartBeats + 0.25, endBeats);
}

void MIDIAudioEngine::Update(Control::AppState& state, const MIDIProject& project, float deltaSeconds) {
    if (!IsActive()) {
        return;
    }

    EnsureSIDReady(state);
    Hardware::SID& sid = state.emulator.GetSID();

    const double clampedDelta = static_cast<double>(std::clamp(deltaSeconds, 0.001F, 0.050F));
    constexpr double SLICE_SEC = 0.001;
    const std::uint32_t cyclesPerMs = isNTSC ? 1000U : 985U;

    int mainChannel = -1;
    int bassChannel = -1;
    DetectProjectChannels(project, mainChannel, bassChannel);

    double remainingSec = clampedDelta;
    while (remainingSec >= (SLICE_SEC * 0.5)) {
        const double stepSec = std::min(remainingSec, SLICE_SEC);
        remainingSec -= stepSec;

        if (playing) {
            playheadSeconds = project.BeatsToSeconds(playheadBeats);
            const double sliceStartSec = playheadSeconds;
            const double sliceEndSec = sliceStartSec + stepSec;

            StepSequencerSlice(sid, project, sliceStartSec, sliceEndSec);

            playheadSeconds = sliceEndSec;
            playheadBeats = project.SecondsToBeats(playheadSeconds);

            if (loopEnabled && loopEndBeats > loopStartBeats && playheadBeats >= loopEndBeats) {
                AllNotesOff(sid);
                playheadBeats = loopStartBeats;
                playheadSeconds = project.BeatsToSeconds(playheadBeats);
            } else if (!loopEnabled) {
                const double totalBeats = std::max(project.GetTotalDurationBeats(), loopEndBeats);
                if (playheadBeats >= totalBeats && !AnyVoiceActive()) {
                    playing = false;
                    playheadBeats = 0.0;
                    playheadSeconds = 0.0;
                }
            }
        }

        if (auditionActive) {
            auditionElapsedSec += stepSec;
            const bool isDrum = (auditionChannel == 9);
            if ((isDrum && auditionElapsedSec >= 0.15) || (auditionReleased && auditionElapsedSec >= 0.12)) {
                TriggerVoiceNoteOff(sid, auditionVoiceIndex);
                auditionActive = false;
                auditionReleased = false;
            }
        }

        effectAccumulatorSec += stepSec;
        while (effectAccumulatorSec >= 0.010) {
            effectAccumulatorSec -= 0.010;
            StepModulationEffects(sid, 0.010, mainChannel);
        }

        sid.Clock(cyclesPerMs);
    }
}

}  // namespace GUI::MIDI
