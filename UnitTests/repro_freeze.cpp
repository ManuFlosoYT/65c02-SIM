#include <iostream>

#include "../Hardware/ACIA.h"
#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "gtest/gtest.h"

using namespace Hardware;

class FreezeRepro : public ::testing::Test {
protected:
    Mem mem;
    CPU cpu;
    ACIA acia;

    void SetUp() override {
        cpu.Reset();
        acia.Init(mem);
        // Load MSBASIC binary manually or assume BIOS is present
        // Since we can't easily link basic.bin here, we will simulate the BIOS
        // read/echo loop logic or try to run the actual BIOS code if possible.
        // Better: Validate ACIA behavior with input.
    }
};

TEST_F(FreezeRepro, InterruptClearsWaiting) {
    // Enable WAI
    cpu.waiting = true;

    // Set IRQ pending in ACIA Status
    mem.memory[ACIA_STATUS] |= 0x80;

    // IRQ logic should clear waiting
    cpu.IRQ(mem);  // Manual trigger

    EXPECT_FALSE(cpu.waiting);
}

TEST_F(FreezeRepro, InstructionStepClearsWaiting) {
    // Enable WAI
    cpu.waiting = true;

    // Set IRQ pending in ACIA Status
    mem.memory[ACIA_STATUS] |= 0x80;

    // Step should detect IRQ and clear waiting
    cpu.Step(mem);

    EXPECT_FALSE(cpu.waiting);
}
