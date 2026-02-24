#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BRA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }
    Mem mem;
    CPU cpu;
};

TEST_F(BRA_Test, BRA_PositiveOffset) {
    cpu.PC = 0x1000;
    mem.Write(0x1000, INS_BRA);
    mem.Write(0x1001, 0x05);     // Offset +5
    mem.Write(0x1007, INS_JAM);  // Stop instruction at 0x1000 + 2 + 5 = 1007

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}

TEST_F(BRA_Test, BRA_NegativeOffset) {
    cpu.PC = 0x1010;
    mem.Write(0x1010, INS_BRA);
    mem.Write(0x1011, 0xFB);  // Offset -5 (0xFB)
    // Destination: 0x1010 + 2 - 5 = 0x1012 - 5 = 0x100D
    mem.Write(0x100D, INS_JAM);  // Stop instruction

    mem.WriteROM(0xFFFC, 0x10);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x100E);
}

TEST_F(BRA_Test, BRA_MaxPositiveOffset) {
    // Jump +127 (0x7F)
    cpu.PC = 0x2000;
    mem.Write(0x2000, INS_BRA);
    mem.Write(0x2001, 0x7F);
    // Dest: 0x2000 + 2 + 127 = 0x2002 + 0x7F = 0x2081
    mem.Write(0x2081, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x20);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x2082);
}

TEST_F(BRA_Test, BRA_MaxNegativeOffset) {
    // Jump -128 (0x80)
    cpu.PC = 0x3000;
    mem.Write(0x3000, INS_BRA);
    mem.Write(0x3001, 0x80);
    // Dest: 0x3000 + 2 - 128 = 0x3002 - 128 = 0x3002 - 0x80 = 0x2F82
    mem.Write(0x2F82, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x30);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x2F83);
}