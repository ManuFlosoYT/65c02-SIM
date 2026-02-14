#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class CPY_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPY_Immediate_Test, CPY_Immediate) {
    // Y = 5, M = 5. Result = 0. Z=1, C=1.
    cpu.Y = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPY_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CPY_Immediate_Test, CPY_Immediate_Less) {
    // Y = 4, M = 5. Result = 0xFF. Z=0, C=0. N=1.
    cpu.Y = 0x04;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPY_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CPY_Immediate_Test, CPY_Immediate_Greater) {
    // Y = 6, M = 5. Result = 1. Z=0, C=1. N=0.
    cpu.Y = 0x06;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPY_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}