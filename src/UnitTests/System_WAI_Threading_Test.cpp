#include "Hardware/Core/Emulator/Emulator.h"

#include <gtest/gtest.h>

using namespace Core;

class System_WAI_Threading_Test : public ::testing::Test {
public:
    Emulator emulator;

    void SetUp() override {
        std::string errorMsg;
        emulator.InitFromMemory({}, "", errorMsg);
        
        
        emulator.GetROM().WriteDirect(0x7FFC, 0x00);
        emulator.GetROM().WriteDirect(0x7FFD, 0x10);
        
        emulator.GetMem().Write(0x1000, 0xCB);
        emulator.GetMem().Write(0x1001, 0xE8);
        emulator.GetMem().Write(0x1002, 0x4C);
        emulator.GetMem().Write(0x1003, 0x02);
        emulator.GetMem().Write(0x1004, 0x10);
        
        emulator.GetROM().WriteDirect(0x7FFE, 0x10);
        emulator.GetROM().WriteDirect(0x7FFF, 0x10);
        emulator.GetMem().Write(0x1010, 0x40);
        
        emulator.Reset();
    }
};

TEST_F(System_WAI_Threading_Test, WAISuspendedUntilInterrupt) {
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false);
    cpu.X = 0;
    
    emulator.Step<false>();
    
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.X, 0);
    
    emulator.Step<false>();
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.X, 0);
    
    emulator.TriggerIRQ();
    
    emulator.Step<false>(); 
    
    EXPECT_FALSE(cpu.waiting);
    
    emulator.Step<false>();
    
    EXPECT_EQ(cpu.PC, 0x1001);
    
    emulator.Step<false>();
    
    EXPECT_EQ(cpu.X, 1);
}
