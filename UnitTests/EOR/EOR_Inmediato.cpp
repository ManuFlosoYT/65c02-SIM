#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class EOR_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_Immediate_Test, EOR_Immediate) {
    cpu.A = 0xFF;  // Start with 0xFF
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_IM);
    mem.Write(0x4001, 0x0F);  // 0xFF ^ 0x0F = 0xF0
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(EOR_Immediate_Test, EOR_Immediate_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_IM);
    mem.Write(0x4001, 0xFF);  // 0xFF ^ 0xFF = 0x00
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(EOR_Immediate_Test, EOR_Immediate_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_IM);
    mem.Write(0x4001, 0x80);  // 0x00 ^ 0x80 = 0x80
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}