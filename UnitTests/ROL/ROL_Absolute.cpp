#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class ROL_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROL_Absolute_Test, ROL_Absolute) {
    // Mem = 0x80 -> 0x00 (Shift Left, C=0 into bit 0).
    // Carry Out = Old Bit 7 (1).
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ROL_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x20);  // 0x2080
    mem.Write(0x2080, 0x80);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x2080], 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
}