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

TEST(MicroDosGPU_Test, PS2InitializationAndFrameEncoding) {
    uint16_t frameAA = PS2MouseController::EncodeFrame(0xAAU);
    EXPECT_EQ(frameAA & 0x01U, 0U);
    EXPECT_EQ((frameAA >> 1U) & 0xFFU, 0xAAU);
    EXPECT_EQ((frameAA >> 10U) & 0x01U, 1U);

    uint8_t decoded = 0U;
    EXPECT_TRUE(PS2MouseController::DecodeFrame(frameAA, decoded));
    EXPECT_EQ(decoded, 0xAAU);
    EXPECT_FALSE(PS2MouseController::DecodeFrame(static_cast<uint16_t>(frameAA ^ (1U << 3U)), decoded));
    EXPECT_FALSE(PS2MouseController::DecodeFrame(static_cast<uint16_t>(frameAA | 0x01U), decoded));

    MicroDosGPU gpu;
    gpu.Reset();
    EXPECT_FALSE(gpu.GetMouse().IsStreamMode());

    bool sawSelfTestAA = false;
    bool sawMouseId00 = false;
    bool sawHostInhibitClk = false;
    bool sawHostTxF4 = false;
    bool sawAckFA = false;

    for (int cycle = 0; cycle < 500 && !gpu.GetMouse().IsStreamMode(); ++cycle) {
        gpu.Clock();
        if (gpu.GetMouse().GetLastRxByte() == 0xAAU) {
            sawSelfTestAA = true;
        }
        if (sawSelfTestAA && gpu.GetMouse().GetLastRxByte() == 0x00U) {
            sawMouseId00 = true;
        }
        if (!gpu.GetMouse().GetPS2Clk()) {
            sawHostInhibitClk = true;
        }
        if (gpu.GetMouse().GetLastHostTxByte() == 0xF4U) {
            sawHostTxF4 = true;
        }
        if (gpu.GetMouse().GetLastRxByte() == 0xFAU) {
            sawAckFA = true;
        }
    }

    EXPECT_TRUE(sawSelfTestAA);
    EXPECT_TRUE(sawMouseId00);
    EXPECT_TRUE(sawHostInhibitClk);
    EXPECT_TRUE(sawHostTxF4);
    EXPECT_TRUE(sawAckFA);
    EXPECT_TRUE(gpu.GetMouse().IsStreamMode());
}

TEST(MicroDosGPU_Test, PS2PacketSyncAndCoordinateClamping) {
    MicroDosGPU gpu;
    gpu.GetMouse().CompleteInit();
    ASSERT_TRUE(gpu.GetMouse().IsStreamMode());

    gpu.GetMouse().ReceiveByte(0x00U);
    EXPECT_EQ(gpu.GetMouse().GetPacketIndex(), 0U);
    EXPECT_FALSE(gpu.GetMouse().IsDirty());

    gpu.GetMouse().ReceiveByte(0x29U);
    EXPECT_EQ(gpu.GetMouse().GetPacketIndex(), 1U);
    gpu.GetMouse().ReceiveByte(25U);
    EXPECT_EQ(gpu.GetMouse().GetPacketIndex(), 2U);
    gpu.GetMouse().ReceiveByte(static_cast<uint8_t>(-20));
    EXPECT_EQ(gpu.GetMouse().GetPacketIndex(), 0U);
    EXPECT_TRUE(gpu.GetMouse().IsDirty());
    EXPECT_EQ(gpu.GetMouse().GetButtons(), MicroDosGPU::MOUSE_BTN_LEFT);
    EXPECT_EQ(gpu.GetMouse().GetPosX(), 25U);
    EXPECT_EQ(gpu.GetMouse().GetPosY(), 20U);

    gpu.GetMouse().InjectPacket(-10, 5, MicroDosGPU::MOUSE_BTN_RIGHT);
    EXPECT_EQ(gpu.GetMouse().GetButtons(), MicroDosGPU::MOUSE_BTN_RIGHT);
    EXPECT_EQ(gpu.GetMouse().GetPosX(), 15U);
    EXPECT_EQ(gpu.GetMouse().GetPosY(), 15U);

    gpu.GetMouse().InjectPacket(200, -200, 0U);
    EXPECT_EQ(gpu.GetMouse().GetPosX(), 99U);
    EXPECT_EQ(gpu.GetMouse().GetPosY(), 74U);

    gpu.GetMouse().InjectPacket(-200, 200, 0U);
    EXPECT_EQ(gpu.GetMouse().GetPosX(), 0U);
    EXPECT_EQ(gpu.GetMouse().GetPosY(), 0U);
}

TEST(MicroDosGPU_Test, BlankingArbiterThreeCycleBurst) {
    MicroDosGPU gpu;
    gpu.GetMouse().CompleteInit();
    ASSERT_FALSE(gpu.IsInBlankingInterval());

    gpu.GetMouse().InjectPacket(45, -30,
                                static_cast<uint8_t>(MicroDosGPU::MOUSE_BTN_LEFT | MicroDosGPU::MOUSE_BTN_RIGHT));
    EXPECT_TRUE(gpu.GetMouse().IsDirty());

    while (gpu.GetPixelX() < 99U) {
        gpu.Clock();
        EXPECT_FALSE(gpu.IsInBlankingInterval());
        EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET), 0U);
        EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_X_OFFSET), 0U);
        EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_Y_OFFSET), 0U);
        EXPECT_TRUE(gpu.GetFpgaRw());
    }

    gpu.Clock();
    EXPECT_EQ(gpu.GetPixelX(), 100U);
    EXPECT_TRUE(gpu.IsInBlankingInterval());
    if (gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET) == 0U) {
        gpu.Clock();
    }

    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET), 0x03U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_X_OFFSET), 0U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_Y_OFFSET), 0U);
    EXPECT_TRUE(gpu.GetMouse().IsDirty());

    gpu.Clock();
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET), 0x03U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_X_OFFSET), 45U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_Y_OFFSET), 0U);
    EXPECT_TRUE(gpu.GetMouse().IsDirty());

    gpu.Clock();
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET), 0x03U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_X_OFFSET), 45U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_Y_OFFSET), 30U);
    EXPECT_FALSE(gpu.GetMouse().IsDirty());
}

TEST(MicroDosGPU_Test, HostMouseInputIntegration) {
    Core::Emulator emu;
    std::string errorMsg;
    emu.InitFromMemory({}, "", errorMsg);

    Core::Cartridge cart;
    cart.loaded = true;
    cart.busDevices = {{"RAM", 0x0000, 0x5FFF}, {"microDOS-GPU", 0x6000, 0x7FFF}, {"ROM", 0x8000, 0xFFFF}};
    emu.SetCartridge(cart);
    emu.SetupHardware();
    emu.SetGPUEnabled(true);

    MicroDosGPU& gpu = emu.GetMicroDosGPU();
    gpu.GetMouse().CompleteInit();
    gpu.UpdateHostMouse(80U, 60U, MicroDosGPU::MOUSE_BTN_MIDDLE);

    for (int i = 0; i < 200; ++i) {
        gpu.Clock();
    }

    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_BTNS_OFFSET), 0x04U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_X_OFFSET), 80U);
    EXPECT_EQ(gpu.Read(MicroDosGPU::MOUSE_Y_OFFSET), 60U);

    EXPECT_EQ(emu.GetMem().ReadDirect(0x7FF0U), 0x04U);
    EXPECT_EQ(emu.GetMem().ReadDirect(0x7FF1U), 80U);
    EXPECT_EQ(emu.GetMem().ReadDirect(0x7FF2U), 60U);
}
