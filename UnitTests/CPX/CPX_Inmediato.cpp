#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class CPX_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPX_Immediate_Test, CPX_Immediate) {
    // X = 5, M = 5. Result = 0. Z=1, C=1.
    cpu.X = 0x05;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPX_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CPX_Immediate_Test, CPX_Immediate_Less) {
    // X = 4, M = 5. Result = 0xFF. Z=0, C=0. N=1.
    cpu.X = 0x04;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPX_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CPX_Immediate_Test, CPX_Immediate_Greater) {
    // X = 6, M = 5. Result = 1. Z=0, C=1. N=0.
    cpu.X = 0x06;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPX_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}