#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Hardware {

struct NSFChannelFrameState {
    uint8_t volume = 0;
    uint16_t period = 0;
    uint8_t duty = 0;
    bool retrigger = false;

    bool operator==(const NSFChannelFrameState&) const = default;
};

struct NSFFrameRecord {
    uint32_t frameIndex = 0;
    std::array<NSFChannelFrameState, 4> channels{};
};

struct NSFRunConfig {
    double maxDurationSec = 0.0;
    int songIndex = 0;
};

struct NSFRunResult {
    bool success = false;
    std::string errorMessage;
    double fps = 60.09881389744051;
    uint32_t totalFrames = 0;
    double durationSec = 0.0;
    std::string stopReason;
    int32_t loopStartFrame = -1;
    int32_t loopEndFrame = -1;
    std::string songName;
    std::string artist;
    std::string copyright;
    std::vector<NSFFrameRecord> frames;
};

class NSFRunner {
public:
    static NSFRunResult RunFile(const std::string& nsfPath, const NSFRunConfig& config = {});
    static NSFRunResult RunFromMemory(std::span<const uint8_t> nsfData, const NSFRunConfig& config = {});
    static bool WriteTraceJson(const NSFRunResult& result, const std::string& outputPath);
    static bool RunAndWriteTrace(const std::string& nsfPath, const std::string& outputPath,
                                 const NSFRunConfig& config = {});
};

}  // namespace Hardware
