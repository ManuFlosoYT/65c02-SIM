#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class TAX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(TAX_Test, TAX) {
    // 0xFFFC: TAX
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x42;
    cpu.X = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TAX_Test, TAX_ZeroFlag) {
    // 0xFFFC: TAX
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x00;
    cpu.X = 0x42;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TAX_Test, TAX_NegativeFlag) {
    // 0xFFFC: TAX
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x80;
    cpu.X = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}