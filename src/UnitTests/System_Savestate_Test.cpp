#include "Hardware/Core/Emulator/Emulator.h"

#include <gtest/gtest.h>
#include <filesystem>

using namespace Core;

class System_Savestate_Test : public ::testing::Test {
public:
    Emulator emulator;
    std::string filename = "test_savestate.bin";

    void SetUp() override {
        std::string errorMsg;
        emulator.InitFromMemory({}, "", errorMsg);
    }

    void TearDown() override {
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
    }
};

TEST_F(System_Savestate_Test, SaveAndLoadState) {
    emulator.GetMem().WriteDirect(0x8000, 0xA9);
    emulator.GetMem().WriteDirect(0x8001, 0x10);
    emulator.GetMem().WriteDirect(0x8002, 0xE8);
    emulator.GetMem().WriteDirect(0x8003, 0xE8);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);

    emulator.Reset();
    emulator.GetCPU().X = 0x41;
    emulator.Step();

    EXPECT_EQ(emulator.GetCPU().A, 0x10);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    EXPECT_TRUE(emulator.SaveState(filename));

    emulator.GetMem().WriteDirect(0x8002, 0xEA);
    emulator.GetCPU().A = 0x00;
    emulator.GetCPU().X = 0x00;
    emulator.GetCPU().Y = 0x00;
    emulator.GetCPU().PC = 0x0000;

    EXPECT_TRUE(emulator.LoadState(filename));

    EXPECT_EQ(emulator.GetMem().ReadDirect(0x8002), 0xE8);
    EXPECT_EQ(emulator.GetCPU().A, 0x10);
    EXPECT_EQ(emulator.GetCPU().X, 0x41);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    // Verify cycle-accurate instruction commit post-restore
    for (int ticks = 0; ticks < 16 && emulator.GetCPU().PC == 0x8002; ++ticks) {
        emulator.Step();
    }
    EXPECT_EQ(emulator.GetCPU().X, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8003);
}

TEST_F(System_Savestate_Test, TimeTravel_StepBackInstructionAndRAM) {
    emulator.GetMem().WriteDirect(0x8000, 0xA9);
    emulator.GetMem().WriteDirect(0x8001, 0x42);
    emulator.GetMem().WriteDirect(0x8002, 0x8D);
    emulator.GetMem().WriteDirect(0x8003, 0x00);
    emulator.GetMem().WriteDirect(0x8004, 0x02);
    emulator.GetMem().WriteDirect(0x8005, 0xE8);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);

    emulator.Reset();
    emulator.SetCycleAccurate(false);
    emulator.GetCPU().PC = 0x8000;
    emulator.GetCPU().isInit = true;
    emulator.GetCPU().A = 0x00;
    emulator.GetCPU().X = 0x10;
    emulator.GetMem().WriteDirect(0x0200, 0x00);

    EXPECT_FALSE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);

    emulator.Step();
    EXPECT_TRUE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    emulator.Step();
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8005);

    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().X, 0x11);

    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().X, 0x10);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8005);
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x42);

    emulator.StepBack();
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x00);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().A, 0x42);

    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);
    EXPECT_EQ(emulator.GetCPU().A, 0x00);
    EXPECT_FALSE(emulator.CanRewind());

    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
}

TEST_F(System_Savestate_Test, TimeTravel_RingBufferCapacityLimit) {
    emulator.GetMem().WriteDirect(0x8000, 0xE8);
    emulator.GetMem().WriteDirect(0x8001, 0x4C);
    emulator.GetMem().WriteDirect(0x8002, 0x00);
    emulator.GetMem().WriteDirect(0x8003, 0x80);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);

    emulator.Reset();
    emulator.GetCPU().X = 0x00;

    for (int i = 0; i < 650; ++i) {
        emulator.Step();
    }

    EXPECT_TRUE(emulator.CanRewind());

    int backSteps = 0;
    while (emulator.CanRewind()) {
        emulator.StepBack();
        backSteps++;
    }
    EXPECT_EQ(backSteps, 600);
}

TEST_F(System_Savestate_Test, TimeTravel_CycleAccurateStepBack) {
    emulator.Reset();
    emulator.SetCycleAccurate(true);
    emulator.GetMem().WriteDirect(0x8000, 0xA9);
    emulator.GetMem().WriteDirect(0x8001, 0x42);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);
    emulator.GetCPU().PC = 0x8000;
    emulator.GetCPU().isInit = true;
    emulator.GetCPU().A = 0x00;
    emulator.ClearRewindBuffer();

    // Multi-cycle rewind: effects on cycle 1, remaining cycles on cycle 2
    emulator.Step();
    EXPECT_TRUE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 1);

    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 0);

    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 1);

    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().A, 0x00);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 0);
    EXPECT_FALSE(emulator.CanRewind());
}
