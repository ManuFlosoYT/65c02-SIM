#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class INY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(INY_Test, INY) {
    // Y = 0x05 -> 0x06
    cpu.Y = 0x05;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INY);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INY_Test, INY_ZeroFlag) {
    // Y = 0xFF -> 0x00
    cpu.Y = 0xFF;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INY);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INY_Test, INY_NegativeFlag) {
    // Y = 0x7F -> 0x80
    cpu.Y = 0x7F;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INY);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}