#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_IndirectY_Test, STA_IndirectY) {
    cpu.A = 0x37;
    cpu.Y = 0x10;

    // 0xFFFC: STA (IndirectY) 0x20
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_INDY);
    mem.Write(0x4001, 0x20);     // Zero Page
    mem.Write(0x4002, INS_JAM);  // Stop

    // Pointer at 0x20 (ZP) -> 0x2070
    mem.Write(0x0020, 0x70);  // Low
    mem.Write(0x0021, 0x20);  // High

    // Effective Address = 0x2070 + 0x10 = 0x2080
    mem.Write(0x2080, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x2080], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}