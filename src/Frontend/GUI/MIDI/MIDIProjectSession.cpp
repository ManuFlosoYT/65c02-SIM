#include "Frontend/GUI/MIDI/MIDIProjectSession.h"
#include "Frontend/GUI/MIDI/MIDIAudioEngine.h"
#include "Frontend/GUI/MIDI/MIDISoundBank.h"
#include "Frontend/Control/AppState.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <filesystem>

namespace GUI::MIDI {

void MIDIProjectSession::PushUndoState() {
    if (undoStack_.size() >= MAX_UNDO_STATES) {
        undoStack_.erase(undoStack_.begin());
    }
    undoStack_.push_back(project_);
    redoStack_.clear();
}

void MIDIProjectSession::RecomputeAllocation(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    MIDIFile::ComputeVoicePreviewAllocation(project_, state.emulator.GetSID().GetModel());
    const double totalBeats = std::max(16.0, std::ceil(project_.GetTotalDurationBeats() / 4.0) * 4.0);
    audioEngine.SetLoopRegion(0.0, totalBeats);
}

void MIDIProjectSession::PerformUndo(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (undoStack_.empty()) {
        return;
    }
    redoStack_.push_back(project_);
    project_ = std::move(undoStack_.back());
    undoStack_.pop_back();
    if (selectedTrackIndex_ >= project_.tracks.size()) {
        selectedTrackIndex_ = project_.tracks.empty() ? 0 : project_.tracks.size() - 1;
    }
    lastSyncedTrackIdx_ = 999999;
    RecomputeAllocation(state, audioEngine);
    statusLog_ = "Undo performed.";
}

void MIDIProjectSession::PerformRedo(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (redoStack_.empty()) {
        return;
    }
    undoStack_.push_back(project_);
    project_ = std::move(redoStack_.back());
    redoStack_.pop_back();
    if (selectedTrackIndex_ >= project_.tracks.size()) {
        selectedTrackIndex_ = project_.tracks.empty() ? 0 : project_.tracks.size() - 1;
    }
    lastSyncedTrackIdx_ = 999999;
    RecomputeAllocation(state, audioEngine);
    statusLog_ = "Redo performed.";
}

void MIDIProjectSession::CopySelectedNotes() {
    if (selectedTrackIndex_ >= project_.tracks.size()) {
        return;
    }
    const auto& track = project_.tracks.at(selectedTrackIndex_);
    copiedNotes_.clear();
    double minBeat = 1e9;
    for (const auto& note : track.notes) {
        if (note.selected) {
            copiedNotes_.push_back(note);
            minBeat = std::min(minBeat, note.startBeat);
        }
    }
    if (!copiedNotes_.empty()) {
        for (auto& note : copiedNotes_) {
            note.startBeat -= minBeat;
        }
        statusLog_ = std::format("Copied {} note(s) to clipboard.", copiedNotes_.size());
    }
}

void MIDIProjectSession::PasteNotes(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (copiedNotes_.empty() || selectedTrackIndex_ >= project_.tracks.size()) {
        return;
    }
    PushUndoState();
    auto& track = project_.tracks.at(selectedTrackIndex_);
    for (auto& note : track.notes) {
        note.selected = false;
    }
    const double baseBeat = audioEngine.GetPlayheadBeats();
    for (const auto& src : copiedNotes_) {
        MIDINote pasted = src;
        pasted.id = project_.nextNoteId++;
        pasted.startBeat = std::max(0.0, baseBeat + src.startBeat);
        pasted.selected = true;
        track.notes.push_back(pasted);
    }
    RecomputeAllocation(state, audioEngine);
    statusLog_ = std::format("Pasted {} note(s) at beat {:.2f}.", copiedNotes_.size(), baseBeat);
}

void MIDIProjectSession::DeleteSelectedNotes(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (selectedTrackIndex_ >= project_.tracks.size()) {
        return;
    }
    auto& track = project_.tracks.at(selectedTrackIndex_);
    const bool anySelected = std::ranges::any_of(track.notes, [](const MIDINote& n) { return n.selected; });
    if (!anySelected) {
        return;
    }
    PushUndoState();
    const auto oldCount = track.notes.size();
    std::erase_if(track.notes, [](const MIDINote& n) { return n.selected; });
    const auto removed = oldCount - track.notes.size();
    RecomputeAllocation(state, audioEngine);
    statusLog_ = std::format("Deleted {} selected note(s).", removed);
}

void MIDIProjectSession::AddTrack(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    PushUndoState();
    MIDITrack newTrack;
    const int nextNum = static_cast<int>(project_.tracks.size()) + 1;
    newTrack.name = std::format("Track {}", nextNum);
    int freeChan = 0;
    for (int candidate = 0; candidate < 16; ++candidate) {
        if (candidate == 9) {
            continue;
        }
        const bool used = std::ranges::any_of(
            project_.tracks, [candidate](const MIDITrack& t) { return t.channel == candidate; });
        if (!used) {
            freeChan = candidate;
            break;
        }
    }
    newTrack.channel = freeChan;
    newTrack.program = 80;
    project_.tracks.push_back(std::move(newTrack));
    selectedTrackIndex_ = project_.tracks.size() - 1;
    lastSyncedTrackIdx_ = 999999;
    RecomputeAllocation(state, audioEngine);
}

void MIDIProjectSession::RemoveTrack(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (project_.tracks.size() <= 1) {
        return;
    }
    PushUndoState();
    project_.tracks.erase(project_.tracks.begin() + static_cast<std::ptrdiff_t>(selectedTrackIndex_));
    if (selectedTrackIndex_ >= project_.tracks.size()) {
        selectedTrackIndex_ = project_.tracks.size() - 1;
    }
    lastSyncedTrackIdx_ = 999999;
    RecomputeAllocation(state, audioEngine);
}

void MIDIProjectSession::ClearTrackNotes(std::size_t trackIdx, Control::AppState& state, MIDIAudioEngine& audioEngine) {
    if (trackIdx >= project_.tracks.size()) {
        return;
    }
    auto& trk = project_.tracks.at(trackIdx);
    if (!trk.notes.empty()) {
        PushUndoState();
        trk.notes.clear();
        RecomputeAllocation(state, audioEngine);
        statusLog_ = "Cleared all notes in active track.";
    }
}

void MIDIProjectSession::SetMainTrack(std::size_t trackIdx, Control::AppState& state, MIDIAudioEngine& audioEngine) {
    PushUndoState();
    if (trackIdx < project_.tracks.size()) {
        auto& track = project_.tracks.at(trackIdx);
        if (track.isMainTrack) {
            track.isMainTrack = false;
            if (track.role == MIDITrackRole::Melody) {
                track.role = MIDITrackRole::Auto;
            }
        } else {
            project_.SetMainTrack(trackIdx);
        }
    }
    RecomputeAllocation(state, audioEngine);
}

void MIDIProjectSession::InitDefaultProject(Control::AppState& state, MIDIAudioEngine& audioEngine) {
    PushUndoState();
    project_.InitDefaultProject();
    selectedTrackIndex_ = 0;
    lastSyncedTrackIdx_ = 999999;
    RecomputeAllocation(state, audioEngine);
    statusLog_ = "Initialized new 3-track SID MIDI project.";
}

bool MIDIProjectSession::LoadFromFile(const std::string& filePath, Control::AppState& state, MIDIAudioEngine& audioEngine) {
    MIDIProject loaded;
    std::string err;
    if (MIDIFile::LoadFromFile(filePath, loaded, err)) {
        PushUndoState();
        project_ = std::move(loaded);
        selectedTrackIndex_ = 0;
        for (std::size_t idx = 0; idx < project_.tracks.size(); ++idx) {
            if (project_.tracks.at(idx).isMainTrack) {
                selectedTrackIndex_ = idx;
                break;
            }
        }
        lastSyncedTrackIdx_ = 999999;
        RecomputeAllocation(state, audioEngine);
        statusLog_ = std::format("Loaded '{}' ({} tracks, {:.0f} BPM)",
                                 std::filesystem::path(filePath).filename().string(), project_.tracks.size(),
                                 project_.bpm);
        return true;
    }
    statusLog_ = "Failed to load MIDI: " + err;
    return false;
}

bool MIDIProjectSession::SaveToFile(const std::string& filePath) {
    std::string err;
    if (MIDIFile::SaveToFile(filePath, project_, err)) {
        project_.filePath = filePath;
        statusLog_ = "Saved MIDI to " + filePath;
        return true;
    }
    statusLog_ = "Failed to save MIDI: " + err;
    return false;
}

} // namespace GUI::MIDI
