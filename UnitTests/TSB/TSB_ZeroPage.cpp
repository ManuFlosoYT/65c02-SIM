#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class TSB_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xAA | 0x55 = 0xFF
    cpu.A = 0xAA;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0x55);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0xFF);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_ClearsZeroFlag) {
    // A = 0x01 (0000 0001)
    // M = 0x01 (0000 0001)
    // A & M = 1 != 0 -> Z = 0
    // M = A | M = 0x01 | 0x01 = 0x01
    cpu.A = 0x01;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0x01);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0x01);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_SetsBits) {
    // A = 0xF0 (1111 0000)
    // M = 0x0F (0000 1111)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xF0 | 0x0F = 0xFF
    cpu.A = 0xF0;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x0020, 0x0F);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x0020], 0xFF);
}