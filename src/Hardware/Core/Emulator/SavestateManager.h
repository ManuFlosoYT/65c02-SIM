#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "Hardware/Core/Emulator/RewindHistoryBuffer.h"

namespace Core {

class Emulator;

enum class SavestateLoadResult : std::uint8_t { Success, HashMismatch, VersionMismatch, StructuralError, GenericError };

class SavestateManager {
   public:
    using SnapshotDelta = Core::SnapshotDelta;

    explicit SavestateManager(Emulator& emulator);
    ~SavestateManager() = default;

    SavestateManager(const SavestateManager&) = delete;
    SavestateManager& operator=(const SavestateManager&) = delete;
    SavestateManager(SavestateManager&&) = delete;
    SavestateManager& operator=(SavestateManager&&) = delete;

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    void Rewind();
    [[nodiscard]] bool CanRewind() const { return rewindHistory.CanRewind(); }
    void StepBack() { Rewind(); }

    [[nodiscard]] SavestateLoadResult GetLastLoadResult() const { return lastLoadResult; }
    [[nodiscard]] std::string GetLastLoadVersion() const { return lastLoadVersion; }

    void SetRewindSnapshotsEnabled(bool enabled) { rewindHistory.SetEnabled(enabled); }
    [[nodiscard]] bool IsRewindSnapshotsEnabled() const { return rewindHistory.IsEnabled(); }
    void SetRewindInterval(int n) { rewindHistory.SetInterval(n); }
    [[nodiscard]] int GetRewindInterval() const { return rewindHistory.GetInterval(); }
    void ClearRewindBuffer() { rewindHistory.Clear(); }

    void SaveStateToBuffer();
    void CapturePreStepSnapshot();
    void OnSliceStep();

    bool SaveComponentsState(std::ostream& stateStream);
    void SaveInternalState(std::ostream& stateStream);
    bool LoadComponentsState(std::istream& stateStream);
    bool LoadInternalState(std::istream& stateStream);

    RewindHistoryBuffer& GetRewindHistory() { return rewindHistory; }
    const RewindHistoryBuffer& GetRewindHistory() const { return rewindHistory; }

   private:
    friend class Emulator;

    Emulator& emulator;

    SavestateLoadResult lastLoadResult = SavestateLoadResult::Success;
    std::string lastLoadVersion;

    RewindHistoryBuffer rewindHistory;
};

}  // namespace Core
