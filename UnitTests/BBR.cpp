#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BBR_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BBRx: Branch if bit x of Accumulator is Reset (0).
// 2-byte instruction: Opcode, Offset.
// Address Mode: Relative

TEST_F(BBR_Test, BBR0_BranchTaken) {
    cpu.PC = 0xFFFC;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BBR0);
    mem.Write(0x4001, 0x05);     // Offset +5
    mem.Write(0x4002, INS_JAM);  // Stop if not taken (or part of skip)

    // Accumulator Value: Bit 0 is 0
    cpu.A = 0xFE;  // 1111 1110

    // Target: PC (after fetch offset at 0xFFFD) = 0xFFFE.
    // 0xFFFE + 5 = 0x0003 (Wraps)
    // We put Stop at 0x0003
    mem.Write(0x4007, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4008);  // 0x0003 + 1 (Stop fetch) = 0x0004
}

TEST_F(BBR_Test, BBR0_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BBR0);
    mem.Write(0x4001, 0x05);     // Offset
    mem.Write(0x4002, INS_JAM);  // Stop if not taken

    // Accumulator Value: Bit 0 is 1
    cpu.A = 0x01;  // 0000 0001 (Bit 0 set)

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}

TEST_F(BBR_Test, BBR7_BranchTaken) {
    cpu.PC = 0xFFFC;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BBR7);
    mem.Write(0x4001, 0x04);     // Offset +4
    mem.Write(0x4002, INS_JAM);  // Stop if not taken

    // Accumulator Value: Bit 7 is 0
    cpu.A = 0x7F;  // 0111 1111

    // Target: PC (0xFFFE) + 4 = 0x0002
    mem.Write(0x4006, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4007);  // 0x0002 + 1 (Stop fetch) = 0x0003
}

TEST_F(BBR_Test, BBR7_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BBR7);
    mem.Write(0x4001, 0x04);
    mem.Write(0x4002, INS_JAM);  // Stop

    // Accumulator Value: Bit 7 is 1
    cpu.A = 0x80;  // 1000 0000

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}