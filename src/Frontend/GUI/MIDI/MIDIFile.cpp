#include "Frontend/GUI/MIDI/MIDIFile.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "Frontend/GUI/MIDI/MIDISoundBank.h"

namespace GUI::MIDI {

namespace {

constexpr const char* META_WAVE_PREFIX = "65C02_WAVE:";

std::uint16_t ReadBE16(const std::vector<std::uint8_t>& buf, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<unsigned int>(buf[offset]) << 8U) |
                                      static_cast<unsigned int>(buf[offset + 1]));
}

std::uint32_t ReadBE32(const std::vector<std::uint8_t>& buf, std::size_t offset) {
    return (static_cast<std::uint32_t>(buf[offset]) << 24U) |
           (static_cast<std::uint32_t>(buf[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(buf[offset + 2]) << 8U) |
           static_cast<std::uint32_t>(buf[offset + 3]);
}

void WriteBE16(std::vector<std::uint8_t>& buf, std::uint16_t val) {
    buf.push_back(static_cast<std::uint8_t>((val >> 8U) & 0xFFU));
    buf.push_back(static_cast<std::uint8_t>(val & 0xFFU));
}

void WriteBE32(std::vector<std::uint8_t>& buf, std::uint32_t val) {
    buf.push_back(static_cast<std::uint8_t>((val >> 24U) & 0xFFU));
    buf.push_back(static_cast<std::uint8_t>((val >> 16U) & 0xFFU));
    buf.push_back(static_cast<std::uint8_t>((val >> 8U) & 0xFFU));
    buf.push_back(static_cast<std::uint8_t>(val & 0xFFU));
}

bool ReadVLQ(const std::vector<std::uint8_t>& data, std::size_t& pos, std::size_t endPos,
             std::uint32_t& outVal) {
    outVal = 0;
    int count = 0;
    while (pos < endPos && count < 4) {
        const std::uint8_t byte = data[pos++];
        outVal = (outVal << 7U) | static_cast<std::uint32_t>(byte & 0x7FU);
        ++count;
        if ((byte & 0x80U) == 0) {
            return true;
        }
    }
    return false;
}

void WriteVLQ(std::vector<std::uint8_t>& out, std::uint64_t val) {
    std::array<std::uint8_t, 5> bytes{};
    std::size_t count = 0;
    bytes.at(count++) = static_cast<std::uint8_t>(val & 0x7FU);
    val >>= 7U;
    while (val > 0 && count < 5) {
        bytes.at(count++) = static_cast<std::uint8_t>((val & 0x7FU) | 0x80U);
        val >>= 7U;
    }
    for (std::size_t i = count; i > 0; --i) {
        out.push_back(bytes.at(i - 1));
    }
}

void WriteMetaString(std::vector<std::uint8_t>& trackBuf, std::uint8_t metaType,
                     const std::string& text) {
    WriteVLQ(trackBuf, 0);
    trackBuf.push_back(0xFFU);
    trackBuf.push_back(metaType);
    WriteVLQ(trackBuf, text.size());
    trackBuf.insert(trackBuf.end(), text.begin(), text.end());
}

std::string CleanTrackName(std::string raw) {
    const std::size_t mainPos = raw.find("[MAIN]");
    if (mainPos != std::string::npos) {
        raw.erase(mainPos, 6);
    }
    while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\0' || raw.back() == '\r' ||
                            raw.back() == '\n')) {
        raw.pop_back();
    }
    return raw;
}

int ComputeNotePriority(const MIDITrack& track, const MIDINote& note) {
    int score = 1000;
    if (track.isMainTrack || track.role == MIDITrackRole::Melody) {
        score = 8000;
    } else if (track.role == MIDITrackRole::Bass) {
        score = 5000;
    } else if (track.channel == 9 || track.role == MIDITrackRole::Percussion) {
        score = 3000;
    }

    if (note.pitch > 80 || note.pitch < 40) {
        score += 100;
    }
    return score + std::clamp(note.pitch, 0, 127);
}

void AnalyzeAndAssignTrackRoles(MIDIProject& project) {
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

}  // namespace

void MIDIProject::InitDefaultProject() {
    filePath.clear();
    bpm = 120.0;
    timeSigNumerator = 4;
    timeSigDenominator = 4;
    ticksPerQuarterNote = 480;
    nextNoteId = 1;
    tracks.clear();

    MIDITrack lead;
    lead.name = "Lead Melody";
    lead.channel = 0;
    lead.program = 80;
    lead.role = MIDITrackRole::Melody;
    lead.isMainTrack = true;
    tracks.push_back(std::move(lead));

    MIDITrack bass;
    bass.name = "Synth Bass";
    bass.channel = 1;
    bass.program = 38;
    bass.role = MIDITrackRole::Bass;
    bass.isMainTrack = false;
    tracks.push_back(std::move(bass));

    MIDITrack harmony;
    harmony.name = "Harmony / Pad";
    harmony.channel = 2;
    harmony.program = 88;
    harmony.role = MIDITrackRole::Harmony;
    harmony.isMainTrack = false;
    tracks.push_back(std::move(harmony));

    MIDITrack drums;
    drums.name = "Drums (Ch 10)";
    drums.channel = 9;
    drums.program = 0;
    drums.role = MIDITrackRole::Percussion;
    drums.isMainTrack = false;
    tracks.push_back(std::move(drums));
}

void MIDIProject::SetMainTrack(std::size_t trackIndex) {
    if (trackIndex >= tracks.size()) {
        return;
    }
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        if (i == trackIndex) {
            tracks[i].isMainTrack = true;
            tracks[i].role = MIDITrackRole::Melody;
        } else {
            if (tracks[i].isMainTrack && tracks[i].role == MIDITrackRole::Melody) {
                tracks[i].role = MIDITrackRole::Auto;
            }
            tracks[i].isMainTrack = false;
        }
    }
}

int MIDIProject::GetMainTrackIndex() const {
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i].isMainTrack) {
            return static_cast<int>(i);
        }
    }
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i].role == MIDITrackRole::Melody) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

double MIDIProject::BeatsToSeconds(double beats) const {
    const double safeBpm = (bpm > 1.0) ? bpm : 120.0;
    return beats * (60.0 / safeBpm);
}

double MIDIProject::SecondsToBeats(double seconds) const {
    const double safeBpm = (bpm > 1.0) ? bpm : 120.0;
    return seconds * (safeBpm / 60.0);
}

double MIDIProject::GetTotalDurationBeats() const {
    double maxBeat = 16.0;
    for (const auto& track : tracks) {
        for (const auto& note : track.notes) {
            maxBeat = std::max(maxBeat, note.startBeat + note.durationBeats);
        }
    }
    return maxBeat;
}

bool MIDIFile::LoadFromFile(const std::string& path, MIDIProject& outProject,
                            std::string& outError) {
    std::ifstream inFile(path, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        outError = "Failed to open MIDI file: " + path;
        return false;
    }

    const std::streamsize fileSize = inFile.tellg();
    if (fileSize <= 0) {
        outError = "MIDI file is empty: " + path;
        return false;
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(fileSize));
    inFile.seekg(0, std::ios::beg);
    if (!inFile.read(std::bit_cast<char*>(bytes.data()), fileSize)) {
        outError = "Failed to read MIDI file contents: " + path;
        return false;
    }

    if (!ParseFromBytes(bytes, outProject, outError)) {
        return false;
    }

    outProject.filePath = path;
    return true;
}

bool MIDIFile::SaveToFile(const std::string& path, const MIDIProject& project,
                          std::string& outError) {
    std::ofstream outFile(path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        outError = "Failed to open file for writing: " + path;
        return false;
    }

    const std::vector<std::uint8_t> bytes = SerializeToBytes(project);
    outFile.write(std::bit_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    if (!outFile.good()) {
        outError = "Failed to write MIDI bytes to: " + path;
        return false;
    }
    return true;
}

bool MIDIFile::ParseFromBytes(const std::vector<std::uint8_t>& data, MIDIProject& outProject,
                              std::string& outError) {
    if (data.size() < 14) {
        outError = "MIDI data is too short to contain a valid MThd header.";
        return false;
    }

    if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd') {
        outError = "Invalid MIDI header magic (expected MThd).";
        return false;
    }

    const std::uint32_t headerLen = ReadBE32(data, 4);
    if (headerLen < 6 || 8 + headerLen > data.size()) {
        outError = "Invalid MThd header length.";
        return false;
    }

    const std::uint16_t format = ReadBE16(data, 8);
    const std::uint16_t numTracksHeader = ReadBE16(data, 10);
    const std::uint16_t divisionRaw = ReadBE16(data, 12);

    if (format > 2) {
        outError = "Unsupported SMF format: " + std::to_string(format);
        return false;
    }

    std::uint16_t tpqn = 480;
    if ((divisionRaw & 0x8000U) == 0 && divisionRaw > 0) {
        tpqn = divisionRaw;
    }

    outProject.filePath.clear();
    outProject.bpm = 120.0;
    outProject.timeSigNumerator = 4;
    outProject.timeSigDenominator = 4;
    outProject.ticksPerQuarterNote = tpqn;
    outProject.tracks.clear();
    outProject.nextNoteId = 1;

    bool tempoFound = false;
    std::size_t cursor = 8 + static_cast<std::size_t>(headerLen);
    std::uint16_t mtrkIndex = 0;

    struct PendingNoteOn {
        std::uint64_t tick = 0;
        int velocity = 100;
    };

    while (cursor + 8 <= data.size()) {
        const bool isMTrk = (data[cursor] == 'M' && data[cursor + 1] == 'T' &&
                             data[cursor + 2] == 'r' && data[cursor + 3] == 'k');
        const std::uint32_t chunkLen = ReadBE32(data, cursor + 4);
        cursor += 8;

        const std::size_t chunkEnd =
            std::min(data.size(), cursor + static_cast<std::size_t>(chunkLen));
        if (!isMTrk) {
            cursor = chunkEnd;
            continue;
        }

        std::size_t pos = cursor;
        std::uint64_t currentTick = 0;
        std::uint8_t runningStatus = 0;

        std::string trackName;
        MIDITrackRole explicitRole = MIDITrackRole::Auto;
        bool explicitMain = false;
        std::uint8_t customWave = 0;

        std::array<int, 16> channelProgram{};
        int firstProgramChannel = 0;
        bool hasAnyProgramChange = false;

        std::array<std::array<std::vector<PendingNoteOn>, 128>, 16> activeNotes{};
        std::array<std::vector<MIDINote>, 16> channelNotes{};

        const auto finishNote = [&](int ch, int pitch, std::uint64_t offTick) {
            auto& queue =
                activeNotes.at(static_cast<std::size_t>(ch)).at(static_cast<std::size_t>(pitch));
            if (queue.empty()) {
                return;
            }
            const PendingNoteOn onEv = queue.front();
            queue.erase(queue.begin());

            MIDINote note;
            note.id = outProject.nextNoteId++;
            note.pitch = pitch;
            note.velocity = onEv.velocity;
            note.startBeat = static_cast<double>(onEv.tick) / static_cast<double>(tpqn);
            const std::uint64_t diffTicks = (offTick > onEv.tick) ? (offTick - onEv.tick) : 0;
            note.durationBeats =
                std::max(0.0625, static_cast<double>(diffTicks) / static_cast<double>(tpqn));
            channelNotes.at(static_cast<std::size_t>(ch)).push_back(note);
        };

        while (pos < chunkEnd) {
            std::uint32_t deltaTicks = 0;
            if (!ReadVLQ(data, pos, chunkEnd, deltaTicks)) {
                break;
            }
            currentTick += deltaTicks;
            if (pos >= chunkEnd) {
                break;
            }

            std::uint8_t status = data[pos];
            if ((status & 0x80U) != 0) {
                ++pos;
                if (status < 0xF0U) {
                    runningStatus = status;
                }
            } else {
                if (runningStatus == 0) {
                    outError = "Invalid MIDI running status byte.";
                    return false;
                }
                status = runningStatus;
            }

            const std::uint8_t msgType = static_cast<std::uint8_t>(status & 0xF0U);
            const int ch = static_cast<int>(status & 0x0FU);

            if (msgType == 0x80U) {
                if (pos + 2 > chunkEnd) {
                    break;
                }
                const int pitch = static_cast<int>(data[pos] & 0x7FU);
                pos += 2;
                finishNote(ch, pitch, currentTick);
            } else if (msgType == 0x90U) {
                if (pos + 2 > chunkEnd) {
                    break;
                }
                const int pitch = static_cast<int>(data[pos] & 0x7FU);
                const int vel = static_cast<int>(data[pos + 1] & 0x7FU);
                pos += 2;
                if (vel == 0) {
                    finishNote(ch, pitch, currentTick);
                } else {
                    activeNotes.at(static_cast<std::size_t>(ch))
                        .at(static_cast<std::size_t>(pitch))
                        .push_back(PendingNoteOn{currentTick, vel});
                }
            } else if (msgType == 0xA0U || msgType == 0xB0U || msgType == 0xE0U) {
                if (pos + 2 > chunkEnd) {
                    break;
                }
                pos += 2;
            } else if (msgType == 0xC0U) {
                if (pos + 1 > chunkEnd) {
                    break;
                }
                const int prog = static_cast<int>(data[pos] & 0x7FU);
                ++pos;
                channelProgram.at(static_cast<std::size_t>(ch)) = prog;
                if (!hasAnyProgramChange) {
                    firstProgramChannel = ch;
                    hasAnyProgramChange = true;
                }
            } else if (msgType == 0xD0U) {
                if (pos + 1 > chunkEnd) {
                    break;
                }
                ++pos;
            } else if (status == 0xF0U || status == 0xF7U) {
                runningStatus = 0;
                std::uint32_t sysexLen = 0;
                if (!ReadVLQ(data, pos, chunkEnd, sysexLen) || pos + sysexLen > chunkEnd) {
                    break;
                }
                pos += sysexLen;
            } else if (status == 0xFFU) {
                if (pos >= chunkEnd) {
                    break;
                }
                const std::uint8_t metaType = data[pos++];
                std::uint32_t metaLen = 0;
                if (!ReadVLQ(data, pos, chunkEnd, metaLen) || pos + metaLen > chunkEnd) {
                    break;
                }

                if (metaType == 0x01U || metaType == 0x03U) {
                    const std::string text(std::bit_cast<const char*>(&data[pos]), metaLen);
                    if (text.find(META_ROLE_MELODY) != std::string::npos ||
                        text.find("[MAIN]") != std::string::npos) {
                        explicitMain = true;
                        explicitRole = MIDITrackRole::Melody;
                    } else if (text.find(META_ROLE_BASS) != std::string::npos) {
                        explicitRole = MIDITrackRole::Bass;
                    } else if (text.find(META_ROLE_HARMONY) != std::string::npos) {
                        explicitRole = MIDITrackRole::Harmony;
                    }

                    if (text.starts_with(META_WAVE_PREFIX)) {
                        const std::string hexPart =
                            text.substr(std::char_traits<char>::length(META_WAVE_PREFIX));
                        char* endPtr = nullptr;
                        const unsigned long waveVal = std::strtoul(hexPart.c_str(), &endPtr, 0);
                        if (endPtr != hexPart.c_str()) {
                            customWave = static_cast<std::uint8_t>(waveVal & 0xFFU);
                        }
                    } else if (metaType == 0x03U && !text.starts_with("65C02_")) {
                        const std::string cleaned = CleanTrackName(text);
                        if (!cleaned.empty()) {
                            trackName = cleaned;
                        }
                    }
                } else if (metaType == 0x51U && metaLen == 3 && !tempoFound) {
                    const std::uint32_t tempoUs =
                        (static_cast<std::uint32_t>(data[pos]) << 16U) |
                        (static_cast<std::uint32_t>(data[pos + 1]) << 8U) |
                        static_cast<std::uint32_t>(data[pos + 2]);
                    if (tempoUs > 0) {
                        outProject.bpm =
                            std::round((60000000.0 / static_cast<double>(tempoUs)) * 100.0) / 100.0;
                        tempoFound = true;
                    }
                } else if (metaType == 0x58U && metaLen >= 2) {
                    outProject.timeSigNumerator = std::max(1, static_cast<int>(data[pos]));
                    outProject.timeSigDenominator = 1 << std::min<int>(data[pos + 1], 6);
                } else if (metaType == 0x2FU) {
                    pos += metaLen;
                    break;
                }

                pos += metaLen;
            } else if (status == 0xF2U) {
                if (pos + 2 > chunkEnd) {
                    break;
                }
                pos += 2;
            } else if (status == 0xF1U || status == 0xF3U) {
                if (pos + 1 > chunkEnd) {
                    break;
                }
                ++pos;
            }
        }

        for (int ch = 0; ch < 16; ++ch) {
            for (int pitch = 0; pitch < 128; ++pitch) {
                while (!activeNotes.at(static_cast<std::size_t>(ch))
                            .at(static_cast<std::size_t>(pitch))
                            .empty()) {
                    finishNote(ch, pitch, currentTick);
                }
            }
        }

        std::vector<int> activeChannels;
        for (int ch = 0; ch < 16; ++ch) {
            if (!channelNotes.at(static_cast<std::size_t>(ch)).empty()) {
                activeChannels.push_back(ch);
            }
        }

        if (activeChannels.size() == 1) {
            const int ch = activeChannels.front();
            MIDITrack track;
            track.channel = ch;
            track.program = channelProgram.at(static_cast<std::size_t>(ch));
            track.name = trackName.empty()
                             ? ((ch == 9)
                                    ? "Drums (Ch 10)"
                                    : ("Track " + std::to_string(outProject.tracks.size() + 1)))
                             : trackName;
            track.role = (ch == 9) ? MIDITrackRole::Percussion : explicitRole;
            track.isMainTrack = explicitMain && (ch != 9);
            track.customWaveOverride = customWave;
            track.notes = std::move(channelNotes.at(static_cast<std::size_t>(ch)));
            std::sort(track.notes.begin(), track.notes.end(),
                      [](const MIDINote& a, const MIDINote& b) {
                          if (std::abs(a.startBeat - b.startBeat) > 1e-6) {
                              return a.startBeat < b.startBeat;
                          }
                          return a.pitch < b.pitch;
                      });
            outProject.tracks.push_back(std::move(track));
        } else if (activeChannels.size() > 1) {
            for (const int ch : activeChannels) {
                MIDITrack track;
                track.channel = ch;
                track.program = channelProgram.at(static_cast<std::size_t>(ch));
                if (ch == 9) {
                    track.name = "Drums (Ch 10)";
                } else if (!trackName.empty()) {
                    track.name = trackName + " (Ch " + std::to_string(ch + 1) + ")";
                } else {
                    track.name = "Channel " + std::to_string(ch + 1);
                }
                track.role = (ch == 9) ? MIDITrackRole::Percussion : explicitRole;
                track.isMainTrack = false;
                track.customWaveOverride = customWave;
                track.notes = std::move(channelNotes.at(static_cast<std::size_t>(ch)));
                std::sort(track.notes.begin(), track.notes.end(),
                          [](const MIDINote& a, const MIDINote& b) {
                              if (std::abs(a.startBeat - b.startBeat) > 1e-6) {
                                  return a.startBeat < b.startBeat;
                              }
                              return a.pitch < b.pitch;
                          });
                outProject.tracks.push_back(std::move(track));
            }
        } else {
            const bool isConductorTrack = (mtrkIndex == 0 && numTracksHeader > 1 &&
                                           !hasAnyProgramChange &&
                                           explicitRole == MIDITrackRole::Auto && !explicitMain);
            if (!isConductorTrack &&
                (hasAnyProgramChange || explicitRole != MIDITrackRole::Auto ||
                 !trackName.empty())) {
                const int ch = hasAnyProgramChange ? firstProgramChannel : 0;
                MIDITrack track;
                track.channel = ch;
                track.program = channelProgram.at(static_cast<std::size_t>(ch));
                track.name = trackName.empty()
                                 ? ("Track " + std::to_string(outProject.tracks.size() + 1))
                                 : trackName;
                track.role = (ch == 9) ? MIDITrackRole::Percussion : explicitRole;
                track.isMainTrack = explicitMain && (ch != 9);
                track.customWaveOverride = customWave;
                outProject.tracks.push_back(std::move(track));
            }
        }

        cursor = chunkEnd;
        ++mtrkIndex;
    }

    if (outProject.tracks.empty()) {
        outProject.InitDefaultProject();
    } else {
        AnalyzeAndAssignTrackRoles(outProject);
    }

    ComputeVoicePreviewAllocation(outProject);
    return true;
}

std::vector<std::uint8_t> MIDIFile::SerializeToBytes(const MIDIProject& project) {
    const std::uint16_t tpqn =
        (project.ticksPerQuarterNote > 0) ? project.ticksPerQuarterNote : 480;
    const auto numTracks = static_cast<std::uint16_t>(project.tracks.size() + 1);

    std::vector<std::uint8_t> out;
    out.reserve(1024);

    // MThd chunk
    out.insert(out.end(), {'M', 'T', 'h', 'd'});
    WriteBE32(out, 6);
    WriteBE16(out, 1);
    WriteBE16(out, numTracks);
    WriteBE16(out, tpqn);

    // Track 0: Conductor track (Time Signature + Tempo)
    {
        std::vector<std::uint8_t> trackBuf;
        std::uint8_t denPow = 2;
        int den = std::max(1, project.timeSigDenominator);
        std::uint8_t powCount = 0;
        while (den > 1 && powCount < 7) {
            den >>= 1;
            ++powCount;
        }
        denPow = powCount;

        WriteVLQ(trackBuf, 0);
        trackBuf.insert(
            trackBuf.end(),
            {0xFFU, 0x58U, 0x04U, static_cast<std::uint8_t>(std::clamp(project.timeSigNumerator, 1, 32)),
             denPow, 0x18U, 0x08U});

        const double safeBpm = (project.bpm > 1.0) ? project.bpm : 120.0;
        const auto tempoUs =
            static_cast<std::uint32_t>(std::llround(60000000.0 / safeBpm));
        WriteVLQ(trackBuf, 0);
        trackBuf.insert(trackBuf.end(),
                        {0xFFU, 0x51U, 0x03U, static_cast<std::uint8_t>((tempoUs >> 16U) & 0xFFU),
                         static_cast<std::uint8_t>((tempoUs >> 8U) & 0xFFU),
                         static_cast<std::uint8_t>(tempoUs & 0xFFU)});

        WriteVLQ(trackBuf, 0);
        trackBuf.insert(trackBuf.end(), {0xFFU, 0x2FU, 0x00U});

        out.insert(out.end(), {'M', 'T', 'r', 'k'});
        WriteBE32(out, static_cast<std::uint32_t>(trackBuf.size()));
        out.insert(out.end(), trackBuf.begin(), trackBuf.end());
    }

    struct RawMidiEvent {
        std::uint64_t tick = 0;
        bool isNoteOn = false;
        std::uint8_t status = 0;
        std::uint8_t data1 = 0;
        std::uint8_t data2 = 0;
    };

    for (const auto& track : project.tracks) {
        std::vector<std::uint8_t> trackBuf;
        const auto ch = static_cast<std::uint8_t>(track.channel & 0x0FU);

        WriteMetaString(trackBuf, 0x03U, track.name.empty() ? "Track" : track.name);

        if (track.isMainTrack || track.role == MIDITrackRole::Melody) {
            WriteMetaString(trackBuf, 0x01U, META_ROLE_MELODY);
        } else if (track.role == MIDITrackRole::Bass) {
            WriteMetaString(trackBuf, 0x01U, META_ROLE_BASS);
        } else if (track.role == MIDITrackRole::Harmony) {
            WriteMetaString(trackBuf, 0x01U, META_ROLE_HARMONY);
        }

        if (track.customWaveOverride != 0) {
            std::ostringstream oss;
            oss << META_WAVE_PREFIX << "0x" << std::uppercase << std::hex << std::setw(2)
                << std::setfill('0') << static_cast<int>(track.customWaveOverride);
            WriteMetaString(trackBuf, 0x01U, oss.str());
        }

        WriteVLQ(trackBuf, 0);
        trackBuf.push_back(static_cast<std::uint8_t>(0xC0U | ch));
        trackBuf.push_back(static_cast<std::uint8_t>(std::clamp(track.program, 0, 127)));

        std::vector<RawMidiEvent> events;
        events.reserve(track.notes.size() * 2);

        for (const auto& note : track.notes) {
            const auto startTick = static_cast<std::uint64_t>(
                std::max<std::int64_t>(0, std::llround(note.startBeat * static_cast<double>(tpqn))));
            const auto durTicks = static_cast<std::uint64_t>(std::max<std::int64_t>(
                1, std::llround(note.durationBeats * static_cast<double>(tpqn))));
            const std::uint64_t endTick = startTick + durTicks;

            const auto pitchByte = static_cast<std::uint8_t>(std::clamp(note.pitch, 0, 127));
            const auto velByte = static_cast<std::uint8_t>(std::clamp(note.velocity, 1, 127));

            events.push_back(RawMidiEvent{startTick, true, static_cast<std::uint8_t>(0x90U | ch),
                                          pitchByte, velByte});
            events.push_back(
                RawMidiEvent{endTick, false, static_cast<std::uint8_t>(0x80U | ch), pitchByte, 0});
        }

        std::sort(events.begin(), events.end(), [](const RawMidiEvent& a, const RawMidiEvent& b) {
            if (a.tick != b.tick) {
                return a.tick < b.tick;
            }
            if (a.isNoteOn != b.isNoteOn) {
                return !a.isNoteOn;
            }
            return a.data1 < b.data1;
        });

        std::uint64_t lastTick = 0;
        for (const auto& ev : events) {
            const std::uint64_t delta = (ev.tick >= lastTick) ? (ev.tick - lastTick) : 0;
            WriteVLQ(trackBuf, delta);
            trackBuf.push_back(ev.status);
            trackBuf.push_back(ev.data1);
            trackBuf.push_back(ev.data2);
            lastTick = ev.tick;
        }

        WriteVLQ(trackBuf, 0);
        trackBuf.insert(trackBuf.end(), {0xFFU, 0x2FU, 0x00U});

        out.insert(out.end(), {'M', 'T', 'r', 'k'});
        WriteBE32(out, static_cast<std::uint32_t>(trackBuf.size()));
        out.insert(out.end(), trackBuf.begin(), trackBuf.end());
    }

    return out;
}

void MIDIFile::ComputeVoicePreviewAllocation(MIDIProject& project, Hardware::SIDModel chipModel) {
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
