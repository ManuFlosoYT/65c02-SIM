#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class TXA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TXA_Test, TXA) {
    // 0xFFFC: TXA
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TXA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x42;
    cpu.A = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TXA_Test, TXA_ZeroFlag) {
    // 0xFFFC: TXA
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TXA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x00;
    cpu.A = 0x42;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TXA_Test, TXA_NegativeFlag) {
    // 0xFFFC: TXA
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TXA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x80;
    cpu.A = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}