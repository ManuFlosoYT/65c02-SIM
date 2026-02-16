#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STX_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STX_Absolute_Test, STX_Absolute) {
    cpu.X = 0x37;

    // 0xFFFC: STX (Absolute) 0x8000
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STX_ABS);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x4003, INS_JAM);  // Stop

    mem.Write(0x8000, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x8000], 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}