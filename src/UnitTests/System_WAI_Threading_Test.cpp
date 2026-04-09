#include "Hardware/Core/Emulator.h"

#include <gtest/gtest.h>

using namespace Core;

class System_WAI_Threading_Test : public ::testing::Test {
public:
    Emulator emulator;

    void SetUp() override {
        std::string errorMsg;
        emulator.InitFromMemory({}, "", errorMsg);
        
        // Let's set up memory to test WAI
        // 0x8000: WAI (0xCB)
        // 0x8001: INX (0xE8)
        // 0x8002: JMP 0x8002 (0x4C 0x02 0x80) -> halt
        
        emulator.GetROM().WriteDirect(0x7FFC, 0x00);
        emulator.GetROM().WriteDirect(0x7FFD, 0x10);
        
        // RAM starts at 0. Write program at 0x1000
        emulator.GetMem().Write(0x1000, 0xCB); // WAI
        emulator.GetMem().Write(0x1001, 0xE8); // INX
        emulator.GetMem().Write(0x1002, 0x4C); // JMP
        emulator.GetMem().Write(0x1003, 0x02);
        emulator.GetMem().Write(0x1004, 0x10);
        
        // Interrupt vector
        emulator.GetROM().WriteDirect(0x7FFE, 0x10);
        emulator.GetROM().WriteDirect(0x7FFF, 0x10);
        emulator.GetMem().Write(0x1010, 0x40); // RTI
        
        // Fully reset emulator
        emulator.Reset();
    }
};

TEST_F(System_WAI_Threading_Test, WAISuspendedUntilInterrupt) {
    auto& cpu = emulator.GetCPU();
    cpu.SetCycleAccurate(false); // Make stepping 1:1 with instructions
    cpu.X = 0;
    
    // Step 1: Execute WAI 
    emulator.Step<false>();
    
    // CPU should now be waiting
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.X, 0); // INX not executed
    
    // Step 2: Try stepping further, should do nothing
    emulator.Step<false>();
    EXPECT_TRUE(cpu.waiting);
    EXPECT_EQ(cpu.X, 0);
    
    // Step 3: Trigger interrupt via Emulator API
    emulator.TriggerIRQ();
    
    // Step 4: Step the emulator. This should handle the IRQ, jump to 0x8010 (RTI), clear waiting
    emulator.Step<false>(); 
    
    EXPECT_FALSE(cpu.waiting); // Waiting is cleared
    
    // Now CPU is executing RTI. It will return from IRQ.
    // Next instruction is the interrupt handler instruction (0x40)
    // Actually the IRQ sets PC = 0x1010, then we step.
    emulator.Step<false>(); // executes RTI at 0x1010
    
    // PC should be back at 0x1001
    EXPECT_EQ(cpu.PC, 0x1001);
    
    // Finally, execute the instruction after WAI (INX)
    emulator.Step<false>(); // executes INX at 0x1001
    
    EXPECT_EQ(cpu.X, 1);
}
