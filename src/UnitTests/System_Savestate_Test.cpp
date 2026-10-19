#include "Hardware/Core/Emulator.h"

#include <gtest/gtest.h>
#include <filesystem>

using namespace Core;

class System_Savestate_Test : public ::testing::Test {
public:
    Emulator emulator;
    std::string filename = "test_savestate.bin";

    void SetUp() override {
        std::string errorMsg;
        // Init with empty ROM
        emulator.InitFromMemory({}, "", errorMsg);
    }

    void TearDown() override {
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
    }
};

TEST_F(System_Savestate_Test, SaveAndLoadState) {
    // Program in ROM: LDA #$10 ; INX ; INX
    emulator.GetMem().WriteDirect(0x8000, 0xA9);
    emulator.GetMem().WriteDirect(0x8001, 0x10);
    emulator.GetMem().WriteDirect(0x8002, 0xE8);
    emulator.GetMem().WriteDirect(0x8003, 0xE8);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);

    // Initialize CPU fetch state and execute first instruction
    emulator.Reset();
    emulator.GetCPU().X = 0x41;
    emulator.Step();

    EXPECT_EQ(emulator.GetCPU().A, 0x10);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    EXPECT_TRUE(emulator.SaveState(filename));

    // Modify state before loading
    emulator.GetMem().WriteDirect(0x8002, 0xEA);
    emulator.GetCPU().A = 0x00;
    emulator.GetCPU().X = 0x00;
    emulator.GetCPU().Y = 0x00;
    emulator.GetCPU().PC = 0x0000;

    EXPECT_TRUE(emulator.LoadState(filename));

    // Check if state is restored
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x8002), 0xE8);
    EXPECT_EQ(emulator.GetCPU().A, 0x10);
    EXPECT_EQ(emulator.GetCPU().X, 0x41);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    // Execute next instruction after restore to catch fetch pointer regressions.
    // In cycle-accurate mode, multiple ticks may be required before commit.
    for (int ticks = 0; ticks < 16 && emulator.GetCPU().PC == 0x8002; ++ticks) {
        emulator.Step();
    }
    EXPECT_EQ(emulator.GetCPU().X, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8003);
}

TEST_F(System_Savestate_Test, TimeTravel_StepBackInstructionAndRAM) {
    // ROM:
    // 0x8000: LDA #$42   (A9 42)
    // 0x8002: STA $0200  (8D 00 02)
    // 0x8005: INX        (E8)
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

    // Initial state
    EXPECT_FALSE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);

    // Step 1: LDA #$42
    emulator.Step();
    EXPECT_TRUE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);

    // Step 2: STA $0200
    emulator.Step();
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8005);

    // Step 3: INX
    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().X, 0x11);

    // Step Back 1: Undoes INX
    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().X, 0x10);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8005);
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x42);

    // Step Back 2: Undoes STA $0200 (RAM at 0x0200 restored)
    emulator.StepBack();
    EXPECT_EQ(emulator.GetMem().ReadDirect(0x0200), 0x00);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().A, 0x42);

    // Step Back 3: Undoes LDA #$42 (A restored to 0x00)
    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);
    EXPECT_EQ(emulator.GetCPU().A, 0x00);
    EXPECT_FALSE(emulator.CanRewind());

    // Step forward again to ensure clean resumption
    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
}

TEST_F(System_Savestate_Test, TimeTravel_RingBufferCapacityLimit) {
    // ROM: INX (0xE8) in loop at 0x8000
    emulator.GetMem().WriteDirect(0x8000, 0xE8);
    emulator.GetMem().WriteDirect(0x8001, 0x4C);
    emulator.GetMem().WriteDirect(0x8002, 0x00);
    emulator.GetMem().WriteDirect(0x8003, 0x80);
    emulator.GetMem().WriteDirect(0xFFFC, 0x00);
    emulator.GetMem().WriteDirect(0xFFFD, 0x80);

    emulator.Reset();
    emulator.GetCPU().X = 0x00;

    // Step 650 instructions (exceeds MAX_REWIND_STATES = 600)
    for (int i = 0; i < 650; ++i) {
        emulator.Step();
    }

    EXPECT_TRUE(emulator.CanRewind());

    // We should be able to step back exactly 600 times
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

    // In cycle-accurate mode, dispatch executes instruction effects on cycle 1 and consumes remaining cycles on cycle 2
    emulator.Step();
    EXPECT_TRUE(emulator.CanRewind());
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 1);

    emulator.Step();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8002);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 0);

    // Step back cycle 2 (restores remainingCycles = 1)
    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().A, 0x42);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 1);

    // Step back cycle 1 (restores pre-dispatch state: A = 0x00, PC = 0x8000)
    emulator.StepBack();
    EXPECT_EQ(emulator.GetCPU().A, 0x00);
    EXPECT_EQ(emulator.GetCPU().PC, 0x8000);
    EXPECT_EQ(emulator.GetCPU().remainingCycles, 0);
    EXPECT_FALSE(emulator.CanRewind());
}
