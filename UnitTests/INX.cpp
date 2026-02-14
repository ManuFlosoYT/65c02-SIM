#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class INX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INX_Test, INX) {
    // X = 0x05 -> 0x06
    cpu.X = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INX);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INX_Test, INX_ZeroFlag) {
    // X = 0xFF -> 0x00
    cpu.X = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INX);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INX_Test, INX_NegativeFlag) {
    // X = 0x7F -> 0x80
    cpu.X = 0x7F;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INX);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}