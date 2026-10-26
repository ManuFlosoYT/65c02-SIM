#include "Frontend/GUI/MIDI/SIDLiveSynthesizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include "Frontend/GUI/MIDI/MIDIVoiceAllocator.h"

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

}  // namespace

void SIDLiveSynthesizer::ResetRegCache() {
    sidRegCache_.fill(-1);
}

void SIDLiveSynthesizer::WriteReg(Hardware::SID& sid, std::uint8_t reg, std::uint8_t val, bool force) {
    reg &= 0x1FU;
    if (force || reg >= 25U || sidRegCache_.at(reg) != static_cast<int>(val)) {
        if (reg < 25U) {
            sidRegCache_.at(reg) = static_cast<int>(val);
        }
        sid.Write(reg, val);
    }
}

std::uint16_t SIDLiveSynthesizer::NoteToSIDFreq(double midiNote) const {
    const double sidClock = isNTSC_ ? CLOCK_NTSC_HZ : CLOCK_PAL_HZ;
    const double hz = 440.0 * std::pow(2.0, (midiNote - 69.0) / 12.0);
    const double freq = std::round((hz * 16777216.0) / sidClock);
    const double clamped = std::clamp(freq, 0.0, 65535.0);
    return static_cast<std::uint16_t>(clamped);
}

std::uint16_t SIDLiveSynthesizer::FreqHzToCutoffDAC(double hz, Hardware::SIDModel model) const {
    if (!isNTSC_ && hz < -1.0) {
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

int SIDLiveSynthesizer::ComputePriority(int note, int channel, MIDITrackRole role, int mainChannel,
                                        int bassChannel) const {
    if (!isNTSC_ && note < -128) {
        return 0;
    }
    const MIDITrackRole effectiveRole =
        MIDIVoiceAllocator::ResolveTrackRole(role, channel, mainChannel, bassChannel);
    return MIDIVoiceAllocator::ComputeNotePriority(note, effectiveRole, false, channel);
}

int SIDLiveSynthesizer::AllocateVoice(int note, int channel, double nowSec, std::uint16_t features,
                                     MIDITrackRole role, int mainChannel, int bassChannel) {
    const MIDITrackRole effectiveRole =
        MIDIVoiceAllocator::ResolveTrackRole(role, channel, mainChannel, bassChannel);
    const bool isPerc = (channel == 9 || effectiveRole == MIDITrackRole::Percussion);

    if (isPerc) {
        int activePerc = 0;
        bool melOrBassActive = false;
        for (const auto& v : voices_) {
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
        const auto& v = voices_.at(static_cast<std::size_t>(i));
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
            const auto& v = voices_.at(static_cast<std::size_t>(i));
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
        voices_.at(modIdx).isModulator = true;
    }

    return chosenIdx;
}

void SIDLiveSynthesizer::TriggerVoiceNoteOn(Hardware::SID& sid, int voiceIdx, int channel, int program, int note,
                                            int velocity, double nowSec, MIDITrackRole role, int mainChannel,
                                            int bassChannel, std::uint8_t customWaveOverride) {
    if (voiceIdx < 0 || voiceIdx >= 3) {
        return;
    }

    const SIDPatchParams params =
        MIDISoundBank::GetInstrumentParams(program, channel, note, velocity, sid.GetModel(), customWaveOverride);
    const MIDITrackRole effectiveRole =
        MIDIVoiceAllocator::ResolveTrackRole(role, channel, mainChannel, bassChannel);

    ActiveSIDVoice& v = voices_.at(static_cast<std::size_t>(voiceIdx));
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
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_AD), 0x00, true);
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_SR), 0x00, true);
    sid.Clock(10U);
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_AD), params.attackDecay, true);
    WriteReg(sid, static_cast<std::uint8_t>(base + REG_SR), params.effectiveSustainRelease, true);

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
        for (const auto& vo : voices_) {
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

            int currVol = sidRegCache_.at(REG_MODE_VOL);
            if (currVol == -1) {
                currVol = 0x0F;
            }
            std::uint8_t fmode = FILT_LP;
            if (HasFeature(params.features, SIDFeatureFlags::FilterHP)) {
                fmode = FILT_HP;
            } else if (HasFeature(params.features, SIDFeatureFlags::FilterBP)) {
                fmode = FILT_BP;
            }
            if (voices_.at(2).isModulator) {
                fmode |= FILT_OFF3;
            }
            const auto modeVol = static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(currVol) & ~(FILT_LP | FILT_BP | FILT_HP | FILT_OFF3)) | fmode);
            WriteReg(sid, REG_MODE_VOL, modeVol);
        }
    } else if (sidRegCache_.at(REG_RESON_FILT) != -1) {
        const auto voiceBit = static_cast<std::uint8_t>(1U << static_cast<unsigned>(voiceIdx));
        const auto currResFilt = static_cast<std::uint8_t>(sidRegCache_.at(REG_RESON_FILT));
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

void SIDLiveSynthesizer::TriggerVoiceNoteOff(Hardware::SID& sid, int voiceIdx) {
    if (voiceIdx < 0 || voiceIdx >= 3) {
        return;
    }
    ActiveSIDVoice& v = voices_.at(static_cast<std::size_t>(voiceIdx));
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

void SIDLiveSynthesizer::TriggerVoiceNoteOffByNoteId(Hardware::SID& sid, std::uint32_t noteId) {
    for (auto& v : voices_) {
        if (v.active && !v.released && v.noteId == noteId) {
            TriggerVoiceNoteOff(sid, v.index);
            break;
        }
    }
}

void SIDLiveSynthesizer::StepModulationEffects(Hardware::SID& sid, double stepSec, int mainChannel) {
    constexpr std::array<int, 3> KICK_OFFSETS = {2, 4, 8};

    for (auto& v : voices_) {
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
                for (const auto& other : voices_) {
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

void SIDLiveSynthesizer::AllNotesOff(Hardware::SID& sid) {
    for (int i = 0; i < 3; ++i) {
        auto& v = voices_.at(static_cast<std::size_t>(i));
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
}

void SIDLiveSynthesizer::ReleaseAllActiveVoices() {
    for (auto& v : voices_) {
        if (v.active && !v.released) {
            v.released = true;
            v.releaseTimerSec = 0.0;
        }
    }
}

void SIDLiveSynthesizer::Reset() {
    sidRegCache_.fill(-1);
    for (auto& v : voices_) {
        v.noteId = 0;
    }
}

bool SIDLiveSynthesizer::AnyVoiceActive() const {
    return voices_[0].active || voices_[1].active || voices_[2].active;
}

}  // namespace GUI::MIDI
