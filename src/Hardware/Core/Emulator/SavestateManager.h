#pragma once

#include <cstdint>
#include <deque>
#include <iosfwd>
#include <mutex>
#include <string>
#include <vector>

namespace Core {

class Emulator;

enum class SavestateLoadResult : std::uint8_t { Success, HashMismatch, VersionMismatch, StructuralError, GenericError };

struct SnapshotDelta {
    struct Chunk {
        uint32_t offset = 0;
        std::vector<uint8_t> xorData;
    };
    std::vector<Chunk> chunks;
    std::string fullState;
    bool isKeyframe = false;
    uint64_t totalCycles = 0;
    uint64_t totalInstructions = 0;
};

class SavestateManager {
   public:
    explicit SavestateManager(Emulator& emulator);
    ~SavestateManager() = default;

    SavestateManager(const SavestateManager&) = delete;
    SavestateManager& operator=(const SavestateManager&) = delete;
    SavestateManager(SavestateManager&&) = delete;
    SavestateManager& operator=(SavestateManager&&) = delete;

    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename, bool forceLoad = false);

    void Rewind();
    [[nodiscard]] bool CanRewind() const;
    void StepBack() { Rewind(); }

    [[nodiscard]] SavestateLoadResult GetLastLoadResult() const { return lastLoadResult; }
    [[nodiscard]] std::string GetLastLoadVersion() const { return lastLoadVersion; }

    void SetRewindSnapshotsEnabled(bool enabled);
    [[nodiscard]] bool IsRewindSnapshotsEnabled() const { return rewindSnapshotsEnabled; }
    void SetRewindInterval(int n);
    [[nodiscard]] int GetRewindInterval() const { return rewindInterval; }
    void ClearRewindBuffer();

    void SaveStateToBuffer();
    void CapturePreStepSnapshot();
    void OnSliceStep();

    bool SaveComponentsState(std::ostream& stateStream);
    void SaveInternalState(std::ostream& stateStream);
    bool LoadComponentsState(std::istream& stateStream);
    bool LoadInternalState(std::istream& stateStream);

   private:
    friend class Emulator;

    Emulator& emulator;

    SavestateLoadResult lastLoadResult = SavestateLoadResult::Success;
    std::string lastLoadVersion;

    std::deque<SnapshotDelta> rewindBuffer;
    std::string lastCapturedState;
    uint64_t lastCapturedCycles = 0;
    uint64_t lastCapturedInstructions = 0;
    mutable std::mutex rewindMutex;
    static constexpr size_t MAX_REWIND_STATES = 600;
    int rewindInterval = 1;
    bool rewindSnapshotsEnabled = true;
    int rewindSliceCounter = 0;
};

}  // namespace Core
