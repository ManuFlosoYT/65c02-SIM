#include "Hardware/Core/Emulator.h"

#include <gtest/gtest.h>
#include <filesystem>

// Stubs for Frontend/Console methods required by Hardware/Core/Emulator
namespace Console {
    bool SaveState(std::ostream& out) { return true; }
    bool LoadState(std::istream& inputStream) { return true; }
}

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
