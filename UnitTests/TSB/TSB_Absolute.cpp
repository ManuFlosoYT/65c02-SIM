#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class TSB_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xFF
    cpu.A = 0xAA;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ABS);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x20);
    mem.WriteROM(0x2000, 0x55);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x2000], 0xFF);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_ClearsZeroFlag) {
    // A = 0x80 (1000 0000)
    // M = 0x80 (1000 0000)
    // A & M != 0 -> Z = 0
    // M = M | A = 0x80
    cpu.A = 0x80;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ABS);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x20);
    mem.WriteROM(0x2000, 0x80);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x2000], 0x80);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsBits) {
    // A = 0x0F (0000 1111)
    // M = 0xF0 (1111 0000)
    // Z = 1
    // M = 0xFF
    cpu.A = 0x0F;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSB_ABS);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x20);
    mem.WriteROM(0x2000, 0xF0);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x2000], 0xFF);
}