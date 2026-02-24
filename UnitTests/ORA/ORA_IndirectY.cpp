#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class ORA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_IndirectY_Test, ORA_IndirectY) {
    cpu.Y = 0x04;
    cpu.A = 0x00;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_INDY);
    mem.Write(0x4001, 0x02);     // ZP
    mem.Write(0x0002, 0x00);     // Low Byte
    mem.Write(0x0003, 0x80);     // High Byte -> Base: 0x8000
    mem.WriteROM(0x8004, 0x0F);  // 0x8000 + 0x04. 0x00 | 0x0F = 0x0F
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}