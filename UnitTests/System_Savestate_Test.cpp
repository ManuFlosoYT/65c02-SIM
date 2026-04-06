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
        emulator.InitFromMemory(nullptr, 0, "", errorMsg);
    }

    void TearDown() override {
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
    }
};

TEST_F(System_Savestate_Test, SaveAndLoadState) {
    // Write something to RAM
    emulator.GetMem().Write(0x1000, 0x42);
    
    // Set some CPU state
    emulator.GetCPU().A = 0xAA;
    emulator.GetCPU().X = 0xBB;
    emulator.GetCPU().Y = 0xCC;
    emulator.GetCPU().PC = 0x1234;

    EXPECT_TRUE(emulator.SaveState(filename));

    // Modify state before loading
    emulator.GetMem().Write(0x1000, 0x00);
    emulator.GetCPU().A = 0x00;
    emulator.GetCPU().X = 0x00;
    emulator.GetCPU().Y = 0x00;
    emulator.GetCPU().PC = 0x0000;

    EXPECT_TRUE(emulator.LoadState(filename));

    // Check if state is restored
    EXPECT_EQ(emulator.GetMem().Read(0x1000), 0x42);
    EXPECT_EQ(emulator.GetCPU().A, 0xAA);
    EXPECT_EQ(emulator.GetCPU().X, 0xBB);
    EXPECT_EQ(emulator.GetCPU().Y, 0xCC);
    EXPECT_EQ(emulator.GetCPU().PC, 0x1234);
}
