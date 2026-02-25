#include <iostream>

#include "Hardware/Comm/ACIA.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Memory/RAM.h"
#include "gtest/gtest.h"

using namespace Hardware;

class FreezeRepro : public ::testing::Test {
protected:
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
    ACIA acia;

    void SetUp() override {
        cpu.Reset();
        bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);  // RAM base
        bus.RegisterDevice(0x5000, 0x5003, &acia, true, true);  // ACIA on top
        acia.Reset();
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
    bus.WriteDirect(ACIA_STATUS, bus.ReadDirect(ACIA_STATUS) | 0x80);

    // IRQ logic should clear waiting
    cpu.IRQ(bus);  // Manual trigger

    EXPECT_FALSE(cpu.waiting);
}

TEST_F(FreezeRepro, InstructionStepClearsWaiting) {
    // Enable WAI
    cpu.waiting = true;

    // Set IRQ pending in ACIA Status
    bus.WriteDirect(ACIA_STATUS, bus.ReadDirect(ACIA_STATUS) | 0x80);

    // Step should detect IRQ and clear waiting
    cpu.Step(bus);

    EXPECT_FALSE(cpu.waiting);
}
