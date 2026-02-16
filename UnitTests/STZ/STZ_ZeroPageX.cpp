#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STZ_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_ZeroPageX_Test, STZ_ZeroPageX_ExecutesCorrectly) {
    cpu.PC = 0xFFFC;
    cpu.X = 0x04;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STZ_ZPX);
    mem.Write(0x4001, 0x20);  // Base 0x20
    mem.Write(0x4002, INS_JAM);

    // Target: 0x20 + 0x04 = 0x24
    mem.Write(0x0024, 0xBB);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0024], 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(STZ_ZeroPageX_Test, STZ_ZeroPageX_Wrapping) {
    cpu.PC = 0xFFFC;
    cpu.X = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STZ_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, INS_JAM);

    // Target: 0x80 + 0xFF = 0x17F -> 0x7F (Zero page wrap)
    mem.Write(0x007F, 0xCC);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x007F], 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}