#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BRK_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(BRK_Test, BRK_Operations) {
    // BRK: Force Interrupt
    // 1. PC + 2 pushed to stack (High byte first)
    // 2. Status Register (P) pushed to stack with Break flag (B) set
    // 3. Interrupt Disable (I) set
    // 4. PC loaded from Interrupt Vector (0xFFFE/0xFFFF)

    cpu.PC = 0x1000;
    cpu.SP = 0x01FF;

    // Interrupt Vector
    bus.WriteDirect(0xFFFE, 0x00);
    bus.WriteDirect(0xFFFF, 0x20);  // 0x2000
    bus.Write(0x2000, INS_JAM);  // STOP

    bus.Write(0x1000, INS_BRK);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    // PC pushed is Address of BRK + 2 = 1002 (Standard 6502)
    // Stack:
    // FF: High PC (0x10)
    // FE: Low PC (0x02)
    // FD: Status (B bit set, I bit not affected)

    EXPECT_EQ(cpu.PC, 0x2001);  // 2000 + 1 (read 0xFF)

    EXPECT_EQ(bus.ReadDirect(0x0100 + 0xFF), 0x10);
    EXPECT_EQ(bus.ReadDirect(0x0100 + 0xFE), 0x02);

    EXPECT_TRUE(cpu.B);  // Break Flag Set
}