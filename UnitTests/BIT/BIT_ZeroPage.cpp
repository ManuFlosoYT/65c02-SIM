#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class BIT_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(BIT_ZeroPage_Test, BIT_ZeroPage_SetsFlagsFromMemory) {
    // Memory value has bit 7 set (N) and bit 6 set (V).
    // A & Mem != 0 -> Z=0
    cpu.A = 0xFF;
    cpu.N = 0;
    cpu.V = 0;
    cpu.Z = 1;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BIT_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0xC0);  // 1100 0000 -> N=1, V=1
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(BIT_ZeroPage_Test, BIT_ZeroPage_SetsZeroFlag) {
    // A & Mem == 0 -> Z=1
    cpu.A = 0x01;
    cpu.Z = 0;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_BIT_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0xFE);  // 1111 1110. A & Mem = 0.
    // Mem also has N=1 (bit 7) and V=1 (bit 6)
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
}