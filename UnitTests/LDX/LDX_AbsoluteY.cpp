#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LDX_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY) {
    cpu.Y = 0x01;
    // 0xFFFC: LDX 0x8000, Y
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0x01 = 0x8001
    // 0x8001: 0x37
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ABSY);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x8001, 0x37);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_PageCrossing) {
    cpu.Y = 0xFF;
    // 0xFFFC: LDX 0x8000, Y
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0xFF = 0x80FF
    // 0x80FF: 0x37
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ABSY);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x80FF, 0x37);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;
    cpu.Y = 0x01;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ABSY);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x8001, 0x00);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;
    cpu.Y = 0x01;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ABSY);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x8001, 0x80);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}