#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

class ROR_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// ROR: Rotate Right through Carry
// [6...0] <- [7...1]
// [7] <- Old Carry
// C <- Old Bit 0

TEST_F(ROR_Accumulator_Test, ROR_Accumulator_Rotate) {
    // 0000 0001 (0x01) -> 0000 0000 (0x00).
    // Carry Out = Old Bit 0 (1).
    // New Bit 7 = Old Carry (0).
    cpu.A = 0x01;
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ROR_A);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ROR_Accumulator_Test, ROR_Accumulator_NoCarryOut) {
    // 0000 0010 (0x02) -> 0000 0001 (0x01).
    // New Bit 7 = Old Carry (1). Result: 1000 0001 (0x81).
    // Carry Out = Old Bit 0 (0).
    cpu.A = 0x02;
    cpu.C = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ROR_A);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x81);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}