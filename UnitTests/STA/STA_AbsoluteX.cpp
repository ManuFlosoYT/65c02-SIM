#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

class STA_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_AbsoluteX_Test, STA_AbsoluteX) {
    cpu.A = 0x37;
    cpu.X = 0x10;

    // 0xFFFC: STA (AbsoluteX) 0x2000
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_ABSX);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x20);
    mem.Write(0x4003, INS_JAM);  // Stop

    // Target = 0x2000 + 0x10 = 0x2010
    mem.Write(0x2010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x2010], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}