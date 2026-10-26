#pragma once

#include <vector>
#include <cstdint>
#include <array>

struct VideoFrameData {
    std::vector<uint8_t> rawPixels;
    std::vector<uint8_t> processedPixels;
    int rawW = 0;
    int rawH = 0;
    int processedW = 0;
    int processedH = 0;
    int64_t pts = 0;
};

class GLPboFrameGrabber {
public:
    GLPboFrameGrabber() = default;
    ~GLPboFrameGrabber();

    GLPboFrameGrabber(const GLPboFrameGrabber&) = delete;
    GLPboFrameGrabber& operator=(const GLPboFrameGrabber&) = delete;
    GLPboFrameGrabber(GLPboFrameGrabber&&) = delete;
    GLPboFrameGrabber& operator=(GLPboFrameGrabber&&) = delete;

    void Initialize(int rawW, int rawH, int processedW, int processedH, bool recordRaw, bool recordProcessed);
    bool Capture(uint32_t texRaw, uint32_t texProcessed, VideoFrameData& outFrame);
    void Cleanup();

private:
    static void SetupPBOs(std::array<uint32_t, 2>& pbos, int width, int height);

    std::array<uint32_t, 2> pboRaw = {0, 0};
    std::array<uint32_t, 2> pboProcessed = {0, 0};
    int pboIndex = 0;
    int pboWarmupFrames = 0;
    size_t pboProcessedCapacity = 0;
    std::array<int, 2> pboProcessedW = {0, 0};
    std::array<int, 2> pboProcessedH = {0, 0};

    int rawWidth = 0;
    int rawHeight = 0;
    int processedWidth = 0;
    int processedHeight = 0;
    bool shouldRecordRaw = true;
    bool shouldRecordProcessed = true;
};
