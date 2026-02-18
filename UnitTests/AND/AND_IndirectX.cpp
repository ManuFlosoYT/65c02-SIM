#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class AND_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_IndirectX_Test, AND_IndirectX) {
    // INS_AND_INDX
    // Operand: Byte (ZP Address)
    // Effective Address = Mem[ZP + X] | (Mem[ZP + X + 1] << 8)
    cpu.X = 0x04;
    cpu.A = 0xFF;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_INDX);
    mem.Write(0x4001,
              0x02);  // ZP Addr = 0x02. Actual Lookup: 0x02 + 0x04 = 0x06
    mem.Write(0x0006, 0x00);  // Low Byte
    mem.Write(0x0007, 0x80);  // High Byte -> Eff Address: 0x8000
    mem.WriteROM(0x8000, 0x37);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}