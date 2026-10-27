#include <gtest/gtest.h>

#include "Hardware/Core/IMemoryBlockDevice.h"
#include "Hardware/Core/SPSCQueue.h"
#include "Hardware/Memory/MemoryPatternInitializer.h"
#include "Hardware/Memory/RAM.h"
#include "Hardware/Memory/ROM.h"
#include "Hardware/Video/RasterTimingGenerator.h"

using namespace Hardware;

TEST(ModularHardwareTest, RasterTimingGeneratorBasic) {
    RasterTimingGenerator rtg;
    rtg.Reset();

    EXPECT_EQ(rtg.pixelX, 0U);
    EXPECT_EQ(rtg.pixelY, 0U);
    EXPECT_FALSE(rtg.IsInBlankingInterval());
    EXPECT_EQ(rtg.GetDrawingWaitCycles(), 99U);

    rtg.SkipDrawingCycles(50);
    EXPECT_EQ(rtg.pixelX, 50U);
    EXPECT_EQ(rtg.GetDrawingWaitCycles(), 49U);

    for (uint32_t i = 0; i < 50; ++i) {
        rtg.Clock();
    }
    EXPECT_EQ(rtg.pixelX, 100U);
    EXPECT_TRUE(rtg.IsInBlankingInterval());
}

TEST(ModularHardwareTest, MemoryPatternInitializer) {
    std::array<uint8_t, 256> mem{};
    mem.fill(0x55);

    MemoryPatternInitializer::Initialize(mem, false);
    for (auto b : mem) {
        EXPECT_EQ(b, 0);
    }

    MemoryPatternInitializer::Initialize(mem, true);
    bool hasNonZero = false;
    for (auto b : mem) {
        if (b != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(ModularHardwareTest, SPSCQueueOperations) {
    SPSCQueue<int, 4> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0U);

    EXPECT_TRUE(queue.push(10));
    EXPECT_TRUE(queue.push(20));
    EXPECT_TRUE(queue.push(30));
    // Capacity is 4, ring buffer holds at most N-1 = 3 elements
    EXPECT_FALSE(queue.push(40));
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 3U);

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 10);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 30);
    EXPECT_FALSE(queue.pop(val));
    EXPECT_TRUE(queue.empty());
}

TEST(ModularHardwareTest, IMemoryBlockDeviceInheritance) {
    RAM ram(1024);
    IMemoryBlockDevice* devRam = &ram;
    EXPECT_NE(devRam->GetRawMemory(), nullptr);
    EXPECT_EQ(devRam->GetRawMemorySize(), 1024U);
    EXPECT_FALSE(devRam->IsReadOnly());

    ROM rom(512);
    IMemoryBlockDevice* devRom = &rom;
    EXPECT_NE(devRom->GetRawMemory(), nullptr);
    EXPECT_EQ(devRom->GetRawMemorySize(), 512U);
    EXPECT_TRUE(devRom->IsReadOnly());
}
