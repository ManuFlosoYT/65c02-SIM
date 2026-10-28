#include "Hardware/Video/MicroDosGPU.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <vector>

#include "Hardware/Core/Emulator/Emulator.h"

using namespace Hardware;

TEST(MicroDosGPU_Test, DimensionsAndName) {
    MicroDosGPU gpu;
    EXPECT_EQ(gpu.GetName(), "microDOS-GPU");
    EXPECT_FALSE(gpu.BlocksCPU());
    EXPECT_EQ(static_cast<int>(MicroDosGPU::VRAM_WIDTH), 100);
    EXPECT_EQ(static_cast<int>(MicroDosGPU::VRAM_HEIGHT), 75);
    EXPECT_EQ(MicroDosGPU::MEMORY_SIZE, 0x2000U);
}

TEST(MicroDosGPU_Test, VisibleVRAMAccess) {
    MicroDosGPU gpu;
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());

    Word addr = (20 << 7) | 50;
    Byte val = 0x42;
    gpu.Write(addr, val);

    EXPECT_EQ(gpu.Read(addr), val);
    EXPECT_EQ(gpu.GetVRAM()[20][50], val);
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());
}

TEST(MicroDosGPU_Test, OffscreenRAMAccess) {
    MicroDosGPU gpu;
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());

    Word addr = (10 << 7) | 105;
    Byte val = 0x5A;
    gpu.Write(addr, val);

    EXPECT_EQ(gpu.Read(addr), val);
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());

    for (size_t x = 0; x < MicroDosGPU::VRAM_WIDTH; ++x) {
        EXPECT_EQ(gpu.GetVRAM()[10][x], 0);
    }

    Word boundaryAddr = 0x1FFF;
    gpu.Write(boundaryAddr, 0xFE);
    EXPECT_EQ(gpu.Read(boundaryAddr), 0xFE);
    EXPECT_FALSE(gpu.ConsumeVRAMDirty());
}

TEST(MicroDosGPU_Test, Reset) {
    MicroDosGPU gpu;
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());

    Word visibleAddr = (20 << 7) | 50;
    Word offscreenAddr = (10 << 7) | 105;
    gpu.Write(visibleAddr, 0xAA);
    gpu.Write(offscreenAddr, 0xBB);

    EXPECT_EQ(gpu.Read(visibleAddr), 0xAA);
    EXPECT_EQ(gpu.Read(offscreenAddr), 0xBB);

    gpu.Reset();

    EXPECT_TRUE(gpu.ConsumeVRAMDirty());

    for (size_t y = 0; y < MicroDosGPU::VRAM_HEIGHT; ++y) {
        for (size_t x = 0; x < MicroDosGPU::VRAM_WIDTH; ++x) {
            EXPECT_EQ(gpu.GetVRAM()[y][x], 0);
        }
    }

    for (const auto b : gpu.GetExtraRAM()) {
        EXPECT_EQ(b, 0);
    }

    EXPECT_EQ(gpu.Read(visibleAddr), 0);
    EXPECT_EQ(gpu.Read(offscreenAddr), 0);
}

TEST(MicroDosGPU_Test, SaveAndLoadState) {
    MicroDosGPU gpu;
    EXPECT_TRUE(gpu.ConsumeVRAMDirty());

    Word visibleAddr1 = (15 << 7) | 30;
    Word visibleAddr2 = (40 << 7) | 80;
    gpu.Write(visibleAddr1, 0x33);
    gpu.Write(visibleAddr2, 0x44);

    Word offscreenAddr1 = (5 << 7) | 110;
    Word offscreenAddr2 = 0x1FFF;
    gpu.Write(offscreenAddr1, 0x55);
    gpu.Write(offscreenAddr2, 0x66);

    for (int i = 0; i < 250; ++i) {
        gpu.Clock();
    }

    std::stringstream ss;
    EXPECT_TRUE(gpu.SaveState(ss));

    MicroDosGPU gpu2;
    EXPECT_TRUE(gpu2.LoadState(ss));

    EXPECT_EQ(gpu2.GetPixelX(), gpu.GetPixelX());
    EXPECT_EQ(gpu2.GetPixelY(), gpu.GetPixelY());
    EXPECT_EQ(gpu2.IsInBlankingInterval(), gpu.IsInBlankingInterval());

    EXPECT_EQ(gpu2.Read(visibleAddr1), 0x33);
    EXPECT_EQ(gpu2.Read(visibleAddr2), 0x44);
    EXPECT_EQ(gpu2.GetVRAM()[15][30], 0x33);
    EXPECT_EQ(gpu2.GetVRAM()[40][80], 0x44);

    EXPECT_EQ(gpu2.Read(offscreenAddr1), 0x55);
    EXPECT_EQ(gpu2.Read(offscreenAddr2), 0x66);
    EXPECT_EQ(gpu2.GetExtraRAM(), gpu.GetExtraRAM());
    EXPECT_EQ(gpu2.GetVRAM(), gpu.GetVRAM());
}

TEST(MicroDosGPU_Test, DualPhaseExecution) {
    Core::Emulator emu;
    std::string errorMsg;
    emu.InitFromMemory({}, "", errorMsg);

    Core::Cartridge cart;
    cart.loaded = true;
    cart.busDevices = {{"RAM", 0x0000, 0x1FFF}, {"microDOS-GPU", 0x2000, 0x3FFF}, {"ROM", 0x8000, 0xFFFF}};
    emu.SetCartridge(cart);
    emu.SetupHardware();
    emu.SetGPUEnabled(true);
    emu.SetRasterBeamSlowMotion(false);

    Hardware::GPU& gpu = emu.GetGPU();
    EXPECT_EQ(gpu.GetName(), "microDOS-GPU");
    EXPECT_FALSE(gpu.BlocksCPU());

    uint16_t initialX = gpu.GetPixelX();
    uint16_t initialY = gpu.GetPixelY();
    EXPECT_EQ(initialX, 0);
    EXPECT_EQ(initialY, 0);
    EXPECT_FALSE(gpu.IsInBlankingInterval());

    emu.Step();

    EXPECT_FALSE(gpu.IsInBlankingInterval());
    EXPECT_GT(gpu.GetPixelX(), initialX);
}
