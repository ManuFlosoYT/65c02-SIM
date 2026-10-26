#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include "Frontend/GUI/MIDI/MIDIFile.h"

namespace Control {
struct AppState;
}

namespace GUI::MIDI {
class MIDIAudioEngine;

class MIDIProjectSession {
public:
    static constexpr std::size_t MAX_UNDO_STATES = 32;

    MIDIProjectSession() = default;

    void PushUndoState();
    void PerformUndo(Control::AppState& state, MIDIAudioEngine& audioEngine);
    void PerformRedo(Control::AppState& state, MIDIAudioEngine& audioEngine);

    void CopySelectedNotes();
    void PasteNotes(Control::AppState& state, MIDIAudioEngine& audioEngine);
    void DeleteSelectedNotes(Control::AppState& state, MIDIAudioEngine& audioEngine);

    void AddTrack(Control::AppState& state, MIDIAudioEngine& audioEngine);
    void RemoveTrack(Control::AppState& state, MIDIAudioEngine& audioEngine);
    void ClearTrackNotes(std::size_t trackIdx, Control::AppState& state, MIDIAudioEngine& audioEngine);
    void SetMainTrack(std::size_t trackIdx, Control::AppState& state, MIDIAudioEngine& audioEngine);

    void RecomputeAllocation(Control::AppState& state, MIDIAudioEngine& audioEngine);

    void InitDefaultProject(Control::AppState& state, MIDIAudioEngine& audioEngine);
    bool LoadFromFile(const std::string& filePath, Control::AppState& state, MIDIAudioEngine& audioEngine);
    bool SaveToFile(const std::string& filePath);

    [[nodiscard]] MIDIProject& GetProject() { return project_; }
    [[nodiscard]] const MIDIProject& GetProject() const { return project_; }

    [[nodiscard]] std::size_t GetSelectedTrackIndex() const { return selectedTrackIndex_; }
    void SetSelectedTrackIndex(std::size_t idx) { selectedTrackIndex_ = idx; }

    [[nodiscard]] const std::vector<MIDINote>& GetCopiedNotes() const { return copiedNotes_; }
    [[nodiscard]] bool CanUndo() const { return !undoStack_.empty(); }
    [[nodiscard]] bool CanRedo() const { return !redoStack_.empty(); }

    [[nodiscard]] const std::string& GetStatusLog() const { return statusLog_; }
    void SetStatusLog(std::string log) { statusLog_ = std::move(log); }

    [[nodiscard]] std::array<char, 128>& GetTrackNameBuf() { return trackNameBuf_; }
    [[nodiscard]] std::size_t& GetLastSyncedTrackIdx() { return lastSyncedTrackIdx_; }

private:
    MIDIProject project_;
    std::size_t selectedTrackIndex_ = 0;

    std::vector<MIDIProject> undoStack_;
    std::vector<MIDIProject> redoStack_;
    std::vector<MIDINote> copiedNotes_;

    std::array<char, 128> trackNameBuf_{};
    std::size_t lastSyncedTrackIdx_ = 999999;

    std::string statusLog_ = "Ready — Click Piano Roll to place & audition SID notes.";
};

} // namespace GUI::MIDI
