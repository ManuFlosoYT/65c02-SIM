#include "Frontend/GUI/MIDI/MIDIVoiceAllocator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include "Frontend/GUI/MIDI/MIDISoundBank.h"

namespace GUI::MIDI {

MIDITrackRole MIDIVoiceAllocator::ResolveTrackRole(MIDITrackRole role, int channel, int mainChannel,
                                                   int bassChannel) {
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

void MIDIVoiceAllocator::DetectProjectChannels(const MIDIProject& project, int& outMainChannel,
                                               int& outBassChannel) {
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

int MIDIVoiceAllocator::ComputeNotePriority(int pitch, MIDITrackRole role, bool isMainTrack,
                                           int channel) {
    const MIDITrackRole effectiveRole =
        (channel == 9 || role == MIDITrackRole::Percussion)
            ? MIDITrackRole::Percussion
            : ((isMainTrack || role == MIDITrackRole::Melody) ? MIDITrackRole::Melody : role);

    int score = 1000;
    if (effectiveRole == MIDITrackRole::Melody) {
        score = 8000;
    } else if (effectiveRole == MIDITrackRole::Bass) {
        score = 5000;
    } else if (effectiveRole == MIDITrackRole::Percussion) {
        score = 3000;
    }

    if (pitch > 80 || pitch < 40) {
        score += 100;
    }
    return score + std::clamp(pitch, 0, 127);
}

int MIDIVoiceAllocator::ComputeNotePriority(const MIDITrack& track, const MIDINote& note) {
    return ComputeNotePriority(note.pitch, track.role, track.isMainTrack, track.channel);
}

void MIDIVoiceAllocator::AnalyzeAndAssignTrackRoles(MIDIProject& project) {
    bool hasMainTrack = false;
    for (auto& track : project.tracks) {
        if (track.channel == 9) {
            track.role = MIDITrackRole::Percussion;
            track.isMainTrack = false;
        }
        if (track.isMainTrack) {
            hasMainTrack = true;
            track.role = MIDITrackRole::Melody;
        }
    }

    if (hasMainTrack) {
        for (auto& track : project.tracks) {
            if (!track.isMainTrack && track.channel != 9 && track.role == MIDITrackRole::Auto) {
                track.role = MIDITrackRole::Harmony;
            }
        }
        return;
    }

    for (auto& track : project.tracks) {
        if (track.channel != 9 && track.role == MIDITrackRole::Melody) {
            track.isMainTrack = true;
            hasMainTrack = true;
            break;
        }
    }

    if (!hasMainTrack) {
        std::vector<std::size_t> candidates;
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            if (project.tracks[i].channel != 9 && project.tracks[i].notes.size() > 10) {
                candidates.push_back(i);
            }
        }
        if (candidates.empty()) {
            for (std::size_t i = 0; i < project.tracks.size(); ++i) {
                if (project.tracks[i].channel != 9 && !project.tracks[i].notes.empty()) {
                    candidates.push_back(i);
                }
            }
        }

        if (!candidates.empty()) {
            const auto avgPitch = [&project](std::size_t idx) {
                double sum = 0.0;
                for (const auto& n : project.tracks[idx].notes) {
                    sum += static_cast<double>(n.pitch);
                }
                return sum / static_cast<double>(project.tracks[idx].notes.size());
            };

            std::size_t bestMelodyIdx = candidates.front();
            std::size_t bestBassIdx = candidates.front();
            double maxAvg = avgPitch(bestMelodyIdx);
            double minAvg = maxAvg;

            for (const std::size_t idx : candidates) {
                const double avg = avgPitch(idx);
                if (avg > maxAvg) {
                    maxAvg = avg;
                    bestMelodyIdx = idx;
                }
                if (avg < minAvg) {
                    minAvg = avg;
                    bestBassIdx = idx;
                }
            }

            project.tracks[bestMelodyIdx].isMainTrack = true;
            project.tracks[bestMelodyIdx].role = MIDITrackRole::Melody;

            if (bestBassIdx != bestMelodyIdx &&
                project.tracks[bestBassIdx].role == MIDITrackRole::Auto) {
                project.tracks[bestBassIdx].role = MIDITrackRole::Bass;
            }
        } else {
            for (auto& track : project.tracks) {
                if (track.channel != 9) {
                    track.isMainTrack = true;
                    track.role = MIDITrackRole::Melody;
                    break;
                }
            }
        }
    }

    for (auto& track : project.tracks) {
        if (!track.isMainTrack && track.channel != 9 && track.role == MIDITrackRole::Auto) {
            track.role = MIDITrackRole::Harmony;
        }
    }
}

void MIDIVoiceAllocator::ComputeVoicePreviewAllocation(MIDIProject& project,
                                                      Hardware::SIDModel chipModel) {
    bool anySolo = false;
    for (const auto& track : project.tracks) {
        if (track.solo) {
            anySolo = true;
            break;
        }
    }

    struct NoteAllocItem {
        MIDINote* note = nullptr;
        double startBeat = 0.0;
        double endBeat = 0.0;
        int priority = 0;
    };

    std::vector<NoteAllocItem> items;
    for (auto& track : project.tracks) {
        const bool trackActive = anySolo ? track.solo : !track.muted;
        if (!trackActive) {
            for (auto& note : track.notes) {
                note.assignedSidVoice = -1;
            }
            continue;
        }

        for (auto& note : track.notes) {
            double durBeats = std::max(0.01, note.durationBeats);
            if (track.channel == 9 || track.role == MIDITrackRole::Percussion) {
                const SIDPatchParams patch = MIDISoundBank::GetInstrumentParams(
                    track.program, 9, note.pitch, note.velocity, chipModel,
                    track.customWaveOverride);
                const double maxDrumBeats =
                    project.SecondsToBeats(static_cast<double>(patch.maxDrumLengthSec));
                if (maxDrumBeats > 0.01) {
                    durBeats = std::min(durBeats, maxDrumBeats);
                }
            }

            items.push_back(NoteAllocItem{&note, note.startBeat, note.startBeat + durBeats,
                                          ComputeNotePriority(track, note)});
        }
    }

    std::sort(items.begin(), items.end(), [](const NoteAllocItem& a, const NoteAllocItem& b) {
        if (std::abs(a.startBeat - b.startBeat) > 1e-6) {
            return a.startBeat < b.startBeat;
        }
        return a.priority > b.priority;
    });

    struct SimVoice {
        bool active = false;
        double endBeat = 0.0;
        int priority = 0;
        MIDINote* activeNote = nullptr;
    };

    std::array<SimVoice, Hardware::SID::MAX_SID_VOICES> voices{};

    for (const auto& item : items) {
        for (auto& v : voices) {
            if (v.active && v.endBeat <= item.startBeat + 1e-5) {
                v.active = false;
                v.activeNote = nullptr;
            }
        }

        int chosenVoice = -1;
        for (int vIdx = 0; vIdx < Hardware::SID::MAX_SID_VOICES; ++vIdx) {
            if (!voices.at(static_cast<std::size_t>(vIdx)).active) {
                chosenVoice = vIdx;
                break;
            }
        }

        if (chosenVoice == -1) {
            int lowestVoice = 0;
            for (int vIdx = 1; vIdx < Hardware::SID::MAX_SID_VOICES; ++vIdx) {
                if (voices.at(static_cast<std::size_t>(vIdx)).priority <
                    voices.at(static_cast<std::size_t>(lowestVoice)).priority) {
                    lowestVoice = vIdx;
                }
            }

            auto& victim = voices.at(static_cast<std::size_t>(lowestVoice));
            if (item.priority > victim.priority) {
                if (victim.activeNote != nullptr &&
                    std::abs(victim.activeNote->startBeat - item.startBeat) < 0.01) {
                    victim.activeNote->assignedSidVoice = -1;
                }
                chosenVoice = lowestVoice;
            }
        }

        if (chosenVoice >= 0) {
            item.note->assignedSidVoice = chosenVoice;
            voices.at(static_cast<std::size_t>(chosenVoice)) =
                SimVoice{true, item.endBeat, item.priority, item.note};
        } else {
            item.note->assignedSidVoice = -1;
        }
    }
}

}  // namespace GUI::MIDI
