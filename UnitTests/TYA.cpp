#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

class TYA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TYA_Test, TYA) {
    // 0xFFFC: TYA
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TYA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x42;
    cpu.A = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_ZeroFlag) {
    // 0xFFFC: TYA
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TYA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x00;
    cpu.A = 0x42;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_NegativeFlag) {
    // 0xFFFC: TYA
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TYA);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x80;
    cpu.A = 0x00;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}