#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class STX_ZeroPageY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STX_ZeroPageY_Test, STX_ZeroPageY) {
    cpu.X = 0x37;
    cpu.Y = 0x0F;

    // 0xFFFC: STX (ZeroPageY) 0x80
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STX_ZPY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, INS_JAM);  // Stop

    // Target Zero Page Address = 0x80 + 0x0F = 0x8F
    mem.Write(0x008F, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x008F], 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}