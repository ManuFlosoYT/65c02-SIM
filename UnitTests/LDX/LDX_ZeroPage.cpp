#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LDX_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage) {
    // 0xFFFC: LDX 0x42
    // 0xFFFD: 0x42
    // 0x0042: 0x37
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x37);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDX_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x80);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}