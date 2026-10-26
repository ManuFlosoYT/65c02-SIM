#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace Core {

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

class RewindHistoryBuffer {
public:
    RewindHistoryBuffer() = default;

    static SnapshotDelta CreateXORDiff(const std::string& oldState, const std::string& newState, uint64_t cycles,
                                       uint64_t instructions);
    static void ApplyXORDiff(std::string& state, const SnapshotDelta& delta);

    void PushState(std::string currentStateStr, uint64_t currentCycles, uint64_t currentInstructions);
    void PushPreStepState(std::string currentStateStr, uint64_t currentCycles, uint64_t currentInstructions);
    bool PopRewind(std::string& outState, uint64_t& outCycles, uint64_t& outInstructions);

    [[nodiscard]] bool CanRewind() const;
    void Clear();

    void SetEnabled(bool enabled);
    [[nodiscard]] bool IsEnabled() const { return rewindSnapshotsEnabled; }

    void SetInterval(int n);
    [[nodiscard]] int GetInterval() const { return rewindInterval; }

    bool ShouldCaptureOnSlice();

private:
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
