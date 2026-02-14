#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class CMP_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_Immediate_Test, CMP_Immediate_Equal) {
    // A = 0x05, M = 0x05. Result = 0. Z=1, C=1.
    cpu.A = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_Immediate_Test, CMP_Immediate_Less) {
    // A = 0x04, M = 0x05. Result = -1 (0xFF). Z=0, C=0. N=1.
    cpu.A = 0x04;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CMP_Immediate_Test, CMP_Immediate_Greater) {
    // A = 0x06, M = 0x05. Result = 1. Z=0, C=1. N=0.
    cpu.A = 0x06;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}