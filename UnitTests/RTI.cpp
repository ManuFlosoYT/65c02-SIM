#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class RTI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(RTI_Test, RTI_Operations) {
    // RTI: Return from Interrupt
    // Pull Status (P), Pull PC.

    // Fake Stack
    cpu.SP = 0x01FC;  // Stack Pointer pointing to empty
    // Pushed values (e.g. from BRK)
    mem.Write(0x0100 + 0xFD, 0b11000000);  // Status (N=1, V=1, others 0)
    mem.Write(0x0100 + 0xFE, 0x02);        // PC Low
    mem.Write(0x0100 + 0xFF, 0x10);        // PC High

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RTI);
    mem.Write(0x4001, INS_JAM);
    mem.Write(0x1002, INS_JAM);  // Return Address

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
    EXPECT_EQ(cpu.SP, 0x01FF);
}