#include <gtest/gtest.h>

#include "Hardware/Core/Emulator.h"

using namespace Core;

TEST(RasterBeam_Test, StepAdvancesPixelByPixelWhenSlowMotionEnabled) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);
    emulator.SetGPUEnabled(true);
    emulator.SetRasterBeamSlowMotion(true);

    Hardware::GPU& gpu = emulator.GetGPU();
    uint16_t initialX = gpu.GetPixelX();
    uint16_t initialY = gpu.GetPixelY();

    emulator.Step();

    EXPECT_EQ(gpu.GetPixelX(), static_cast<uint16_t>(initialX + 1));
    EXPECT_EQ(gpu.GetPixelY(), initialY);
}

TEST(RasterBeam_Test, StepFastForwardsToBlankingWhenSlowMotionDisabled) {
    Emulator emulator;
    std::string errorMsg;
    emulator.InitFromMemory({}, "", errorMsg);
    emulator.SetGPUEnabled(true);
    emulator.SetRasterBeamSlowMotion(false);

    Hardware::GPU& gpu = emulator.GetGPU();
    emulator.Step();

    EXPECT_TRUE(gpu.IsInBlankingInterval());
}
