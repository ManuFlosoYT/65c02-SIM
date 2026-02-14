#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class AND_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_ZeroPage_Test, AND_ZeroPage) {
    // 0xFFFC: AND (ZP) 0x42
    // 0x0042: 0x37
    cpu.A = 0xFF;  // Start with A = 0xFF
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x37);  // Value to AND with
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x37);  // 0xFF & 0x37 = 0x37
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_ZeroPage_Test, AND_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xF0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x0F);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_ZeroPage_Test, AND_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x80);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}