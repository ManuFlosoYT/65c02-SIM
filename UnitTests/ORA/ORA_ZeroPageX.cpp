#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class ORA_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_ZeroPageX_Test, ORA_ZeroPageX) {
    cpu.X = 0x05;
    cpu.A = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x0F);  // 0x42 + 0x05 = 0x47. 0x00 | 0x0F = 0x0F
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}