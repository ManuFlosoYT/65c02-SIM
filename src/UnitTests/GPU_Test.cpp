#include "Hardware/Video/GPU.h"

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

using namespace Hardware;

TEST(GPU_Test, Dimensions) {
    EXPECT_EQ(static_cast<int>(GPU::VRAM_WIDTH), 100);
    EXPECT_EQ(static_cast<int>(GPU::VRAM_HEIGHT), 75);
    EXPECT_EQ(static_cast<int>(GPU::DISPLAY_WIDTH), 132);
    EXPECT_EQ(static_cast<int>(GPU::DISPLAY_HEIGHT), 78);
    EXPECT_EQ(static_cast<int>(GPU::VRAM_HEIGHT_DRAWABLE_BY_CPU), 64);

    GPU gpu;
    EXPECT_EQ(gpu.GetName(), "GPU");
}

TEST(GPU_Test, VRAMReadWriteAndDirtyTracking) {
    GPU gpu;

    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());

    Word addrValid = 42 | (25 << 7);
    gpu.Write(addrValid, 0xAB);
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());
    EXPECT_EQ(gpu.Read(addrValid), 0xAB);
    EXPECT_EQ(gpu.ReadDirect(addrValid), 0xAB);
    EXPECT_EQ(gpu.GetVRAM()[25][42], 0xAB);

    Word addrCorner = 99 | (74 << 7);
    gpu.WriteDirect(addrCorner, 0xCD);
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
    EXPECT_EQ(gpu.Read(addrCorner), 0xCD);
    EXPECT_EQ(gpu.GetVRAM()[74][99], 0xCD);

    Word outX = 100 | (10 << 7);
    gpu.Write(outX, 0xEE);
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());
    EXPECT_EQ(gpu.Read(outX), 0);
    EXPECT_EQ(gpu.ReadDirect(outX), 0);

    Word outY = 10 | (75 << 7);
    gpu.Write(outY, 0xFF);
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());
    EXPECT_EQ(gpu.Read(outY), 0);
    EXPECT_EQ(gpu.ReadDirect(outY), 0);

    Word outXY = 127 | (127 << 7);
    gpu.Write(outXY, 0x55);
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());
    EXPECT_EQ(gpu.Read(outXY), 0);
}

TEST(GPU_Test, LoadVRAMSpan) {
    GPU gpu;
    std::vector<uint8_t> buffer(128 * GPU::VRAM_HEIGHT, 0);
    for (size_t y = 0; y < GPU::VRAM_HEIGHT; ++y) {
        for (size_t x = 0; x < GPU::VRAM_WIDTH; ++x) {
            buffer[y * 128 + x] = static_cast<uint8_t>((x + y * 3) & 0xFF);
        }
    }

    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
    gpu.LoadVRAM(buffer);
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());

    for (size_t y = 0; y < GPU::VRAM_HEIGHT; ++y) {
        for (size_t x = 0; x < GPU::VRAM_WIDTH; ++x) {
            EXPECT_EQ(gpu.GetVRAM()[y][x], static_cast<Byte>((x + y * 3) & 0xFF));
        }
    }

    gpu.Reset();
    std::vector<uint8_t> span7500(7500);
    for (size_t i = 0; i < span7500.size(); ++i) {
        span7500[i] = static_cast<uint8_t>((i & 0x7F) + 1);
    }
    gpu.LoadVRAM(span7500);

    for (size_t y = 0; y < GPU::VRAM_HEIGHT; ++y) {
        for (size_t x = 0; x < GPU::VRAM_WIDTH; ++x) {
            size_t addr = y * 128 + x;
            if (addr < span7500.size()) {
                EXPECT_EQ(gpu.GetVRAM()[y][x], span7500[addr]);
            } else {
                EXPECT_EQ(gpu.GetVRAM()[y][x], 0);
            }
        }
    }
}

TEST(GPU_Test, BlankingIntervalsAndScanning) {
    GPU gpu;
    EXPECT_EQ(gpu.GetPixelX(), 0);
    EXPECT_EQ(gpu.GetPixelY(), 0);
    EXPECT_FALSE(gpu.IsInBlankingInterval());

    for (int i = 0; i < 99; ++i) {
        gpu.Clock();
    }
    EXPECT_EQ(gpu.GetPixelX(), 99);
    EXPECT_EQ(gpu.GetPixelY(), 0);
    EXPECT_FALSE(gpu.IsInBlankingInterval());

    gpu.Clock();
    EXPECT_EQ(gpu.GetPixelX(), 100);
    EXPECT_EQ(gpu.GetPixelY(), 0);
    EXPECT_TRUE(gpu.IsInBlankingInterval());

    for (int i = 0; i < 32; ++i) {
        gpu.Clock();
    }
    EXPECT_EQ(gpu.GetPixelX(), 0);
    EXPECT_EQ(gpu.GetPixelY(), 1);
    EXPECT_FALSE(gpu.IsInBlankingInterval());

    for (int i = 0; i < 63 * 132; ++i) {
        gpu.Clock();
    }
    EXPECT_EQ(gpu.GetPixelX(), 0);
    EXPECT_EQ(gpu.GetPixelY(), 64);
    EXPECT_TRUE(gpu.IsInBlankingInterval());

    for (int i = 0; i < 50; ++i) {
        gpu.Clock();
    }
    EXPECT_EQ(gpu.GetPixelX(), 50);
    EXPECT_EQ(gpu.GetPixelY(), 64);
    EXPECT_TRUE(gpu.IsInBlankingInterval());

    for (int i = 0; i < (82 + 13 * 132); ++i) {
        gpu.Clock();
    }
    EXPECT_EQ(gpu.GetPixelX(), 0);
    EXPECT_EQ(gpu.GetPixelY(), 0);
    EXPECT_FALSE(gpu.IsInBlankingInterval());
}

TEST(GPU_Test, DrawingWaitCyclesAndSkip) {
    GPU gpu;
    EXPECT_EQ(gpu.GetDrawingWaitCycles(), 99U);

    gpu.SkipDrawingCycles(50);
    EXPECT_EQ(gpu.GetPixelX(), 50);
    EXPECT_EQ(gpu.GetDrawingWaitCycles(), 49U);

    gpu.SkipDrawingCycles(49);
    EXPECT_EQ(gpu.GetPixelX(), 99);
    EXPECT_EQ(gpu.GetDrawingWaitCycles(), 0U);

    gpu.Clock();
    EXPECT_EQ(gpu.GetPixelX(), 100);
    EXPECT_TRUE(gpu.IsInBlankingInterval());
    EXPECT_EQ(gpu.GetDrawingWaitCycles(), 0U);
}

TEST(GPU_Test, Serialization) {
    GPU gpu1;
    gpu1.Write(10 | (5 << 7), 0x44);
    for (int i = 0; i < 40; ++i) {
        gpu1.Clock();
    }

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    EXPECT_TRUE(gpu1.SaveState(ss));

    GPU gpu2;
    EXPECT_TRUE(gpu2.LoadState(ss));

    EXPECT_EQ(gpu2.GetPixelX(), 40);
    EXPECT_EQ(gpu2.GetPixelY(), 0);
    EXPECT_FALSE(gpu2.IsInBlankingInterval());
    EXPECT_EQ(gpu2.Read(10 | (5 << 7)), 0x44);
    EXPECT_TRUE(gpu2.ConsumeVRAMDirty());

    for (int i = 0; i < 60; ++i) {
        gpu2.Clock();
    }
    EXPECT_EQ(gpu2.GetPixelX(), 100);
    EXPECT_TRUE(gpu2.IsInBlankingInterval());

    GPU gpu3;
    for (int i = 0; i < 110; ++i) {
        gpu3.Clock();
    }
    std::stringstream ss2(std::ios::in | std::ios::out | std::ios::binary);
    EXPECT_TRUE(gpu3.SaveState(ss2));

    GPU gpu4;
    EXPECT_TRUE(gpu4.LoadState(ss2));
    EXPECT_EQ(gpu4.GetPixelX(), 110);
    EXPECT_TRUE(gpu4.IsInBlankingInterval());

    for (int i = 0; i < 22; ++i) {
        gpu4.Clock();
    }
    EXPECT_EQ(gpu4.GetPixelX(), 0);
    EXPECT_EQ(gpu4.GetPixelY(), 1);
    EXPECT_FALSE(gpu4.IsInBlankingInterval());

    GPU gpu5;
    for (int i = 0; i < (65 * 132 + 20); ++i) {
        gpu5.Clock();
    }
    std::stringstream ss3(std::ios::in | std::ios::out | std::ios::binary);
    EXPECT_TRUE(gpu5.SaveState(ss3));

    GPU gpu6;
    EXPECT_TRUE(gpu6.LoadState(ss3));
    EXPECT_EQ(gpu6.GetPixelX(), 20);
    EXPECT_EQ(gpu6.GetPixelY(), 65);
    EXPECT_TRUE(gpu6.IsInBlankingInterval());
}
