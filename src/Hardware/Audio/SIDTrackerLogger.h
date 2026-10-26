#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

namespace Hardware {

struct SIDTrackerFrame {
    uint32_t frameNumber = 0;
    std::array<uint8_t, 0x20> registers{};
    uint32_t writeMask = 0;
};

struct SIDTrackerLogger {
    static constexpr size_t TRACKER_LOG_CAPACITY = 512;

    mutable std::mutex trackerMutex;
    std::vector<SIDTrackerFrame> trackerLog;
    SIDTrackerFrame currentTrackerFrame;
    uint64_t lastTrackerFrameClock = 0;

    void Reset() {
        std::lock_guard<std::mutex> lock(trackerMutex);
        trackerLog.clear();
        currentTrackerFrame = {};
        lastTrackerFrameClock = 0;
    }

    void LogWrite(uint8_t reg, uint8_t data, bool emulationPaused) {
        std::lock_guard<std::mutex> lock(trackerMutex);
        currentTrackerFrame.registers.at(reg) = data;
        currentTrackerFrame.writeMask |= (1U << reg);
        if (emulationPaused) {
            trackerLog.push_back(currentTrackerFrame);
            if (trackerLog.size() > TRACKER_LOG_CAPACITY) {
                trackerLog.erase(trackerLog.begin());
            }
            currentTrackerFrame.writeMask = 0;
            currentTrackerFrame.frameNumber++;
        }
    }

    void StepFrame(uint64_t clockCounter, uint64_t framePeriod) {
        if (framePeriod > 0 && clockCounter - lastTrackerFrameClock >= framePeriod) {
            lastTrackerFrameClock = clockCounter;
            std::lock_guard<std::mutex> lock(trackerMutex);
            if (currentTrackerFrame.writeMask != 0) {
                trackerLog.push_back(currentTrackerFrame);
                if (trackerLog.size() > TRACKER_LOG_CAPACITY) {
                    trackerLog.erase(trackerLog.begin());
                }
                currentTrackerFrame.writeMask = 0;
            }
            currentTrackerFrame.frameNumber++;
        }
    }

    void GetTrackerLog(std::vector<SIDTrackerFrame>& outLog) const {
        std::lock_guard<std::mutex> lock(trackerMutex);
        outLog = trackerLog;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(trackerMutex);
        trackerLog.clear();
        currentTrackerFrame.writeMask = 0;
    }
};

}  // namespace Hardware
