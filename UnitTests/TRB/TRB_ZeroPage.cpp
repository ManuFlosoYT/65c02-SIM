#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

class TRB_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TRB_ZeroPage_Test, TRB_ZeroPage_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = M & ~A = 0x55 & 0x55 = 0x55
    cpu.A = 0xAA;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TRB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0x55);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0x55);
}

TEST_F(TRB_ZeroPage_Test, TRB_ZeroPage_ClearsZeroFlag_And_ResetsBits) {
    // A = 0xFF (1111 1111)
    // M = 0xFF (1111 1111)
    // A & M = 0xFF != 0 -> Z = 0
    // M = M & ~A = 0xFF & 0x00 = 0x00
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TRB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0xFF);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0x00);
}

TEST_F(TRB_ZeroPage_Test, TRB_ZeroPage_PartialReset) {
    // A = 0xF0 (1111 0000)
    // M = 0xAA (1010 1010)
    // A & M = 0xA0 (1010 0000) != 0 -> Z = 0
    // M = M & ~A = 0xAA & 0x0F = 0x0A (0000 1010)
    cpu.A = 0xF0;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TRB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0xAA);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0x0A);
}