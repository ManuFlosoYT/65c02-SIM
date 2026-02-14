#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class DEC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage) {
    // Mem[0x42] = 0x05 -> 0x04
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage_ZeroFlag) {
    // Mem[0x42] = 0x01 -> 0x00
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x01);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage_NegativeFlag) {
    // Mem[0x42] = 0x00 -> 0xFF
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}