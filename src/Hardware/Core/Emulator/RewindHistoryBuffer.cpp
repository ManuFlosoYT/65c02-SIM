#include "Hardware/Core/Emulator/RewindHistoryBuffer.h"

#include <utility>

namespace Core {

SnapshotDelta RewindHistoryBuffer::CreateXORDiff(const std::string& oldState, const std::string& newState, uint64_t cycles,
                                                uint64_t instructions) {
    SnapshotDelta delta;
    delta.totalCycles = cycles;
    delta.totalInstructions = instructions;

    if (oldState.empty() || oldState.size() != newState.size()) {
        delta.isKeyframe = true;
        delta.fullState = oldState;
        return delta;
    }

    const size_t len = oldState.size();
    size_t i = 0;
    size_t totalDiffBytes = 0;

    while (i < len) {
        if (oldState.at(i) == newState.at(i)) {
            ++i;
            continue;
        }
        size_t start = i;
        while (i < len && oldState.at(i) != newState.at(i)) {
            ++i;
        }

        SnapshotDelta::Chunk chunk;
        chunk.offset = static_cast<uint32_t>(start);
        chunk.xorData.resize(i - start);
        for (size_t j = 0; j < chunk.xorData.size(); ++j) {
            chunk.xorData.at(j) = static_cast<uint8_t>(oldState.at(start + j) ^ newState.at(start + j));
        }
        totalDiffBytes += chunk.xorData.size();
        delta.chunks.push_back(std::move(chunk));
    }

    if (totalDiffBytes > (len * 4) / 10) {
        delta.chunks.clear();
        delta.isKeyframe = true;
        delta.fullState = oldState;
    }

    return delta;
}

void RewindHistoryBuffer::ApplyXORDiff(std::string& state, const SnapshotDelta& delta) {
    if (delta.isKeyframe) {
        state = delta.fullState;
    } else {
        for (const auto& chunk : delta.chunks) {
            if (chunk.offset + chunk.xorData.size() <= state.size()) {
                for (size_t j = 0; j < chunk.xorData.size(); ++j) {
                    state.at(chunk.offset + j) ^= static_cast<char>(chunk.xorData.at(j));
                }
            }
        }
    }
}

void RewindHistoryBuffer::PushState(std::string currentStateStr, uint64_t currentCycles, uint64_t currentInstructions) {
    std::scoped_lock lock(rewindMutex);
    if (lastCapturedState.empty()) {
        lastCapturedState = std::move(currentStateStr);
        lastCapturedCycles = currentCycles;
        lastCapturedInstructions = currentInstructions;
        return;
    }

    SnapshotDelta delta =
        CreateXORDiff(lastCapturedState, currentStateStr, lastCapturedCycles, lastCapturedInstructions);
    rewindBuffer.push_back(std::move(delta));
    if (rewindBuffer.size() > MAX_REWIND_STATES) {
        rewindBuffer.pop_front();
    }

    lastCapturedState = std::move(currentStateStr);
    lastCapturedCycles = currentCycles;
    lastCapturedInstructions = currentInstructions;
}

void RewindHistoryBuffer::PushPreStepState(std::string currentStateStr, uint64_t currentCycles, uint64_t currentInstructions) {
    std::scoped_lock lock(rewindMutex);
    if (lastCapturedState.empty()) {
        lastCapturedState = std::move(currentStateStr);
        lastCapturedCycles = currentCycles;
        lastCapturedInstructions = currentInstructions;
        return;
    }

    if (currentStateStr != lastCapturedState) {
        SnapshotDelta delta =
            CreateXORDiff(lastCapturedState, currentStateStr, lastCapturedCycles, lastCapturedInstructions);
        rewindBuffer.push_back(std::move(delta));
        if (rewindBuffer.size() > MAX_REWIND_STATES) {
            rewindBuffer.pop_front();
        }
        lastCapturedState = std::move(currentStateStr);
        lastCapturedCycles = currentCycles;
        lastCapturedInstructions = currentInstructions;
    }
}

bool RewindHistoryBuffer::PopRewind(std::string& outState, uint64_t& outCycles, uint64_t& outInstructions) {
    std::scoped_lock lock(rewindMutex);
    if (rewindBuffer.empty()) {
        return false;
    }

    SnapshotDelta delta = std::move(rewindBuffer.back());
    rewindBuffer.pop_back();

    ApplyXORDiff(lastCapturedState, delta);

    lastCapturedCycles = delta.totalCycles;
    lastCapturedInstructions = delta.totalInstructions;
    outState = lastCapturedState;
    outCycles = delta.totalCycles;
    outInstructions = delta.totalInstructions;
    return true;
}

bool RewindHistoryBuffer::CanRewind() const {
    std::scoped_lock lock(rewindMutex);
    return !rewindBuffer.empty();
}

void RewindHistoryBuffer::Clear() {
    std::scoped_lock lock(rewindMutex);
    rewindBuffer.clear();
    lastCapturedState.clear();
    lastCapturedCycles = 0;
    lastCapturedInstructions = 0;
    rewindSliceCounter = 0;
}

void RewindHistoryBuffer::SetEnabled(bool enabled) {
    rewindSnapshotsEnabled = enabled;
    if (!rewindSnapshotsEnabled) {
        Clear();
    }
}

void RewindHistoryBuffer::SetInterval(int n) {
    if (n > 0) {
        rewindInterval = n;
    }
}

bool RewindHistoryBuffer::ShouldCaptureOnSlice() {
    if (rewindSnapshotsEnabled) {
        rewindSliceCounter++;
        if (rewindSliceCounter >= rewindInterval) {
            rewindSliceCounter = 0;
            return true;
        }
    }
    return false;
}

}  // namespace Core
