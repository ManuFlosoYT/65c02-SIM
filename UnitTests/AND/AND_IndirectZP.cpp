#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class AND_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_IndirectZP_Test, AND_IndirectZP) {
    // INS_AND_IND_ZP
    // Operand: Byte (ZP Address of Pointer)
    // Effective Address = Mem[ZP] | (Mem[ZP + 1] << 8)
    // No indexing
    cpu.A = 0xFF;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_IND_ZP);
    mem.Write(0x4001, 0x20);     // ZP Pointer
    mem.Write(0x0020, 0x00);     // Low Byte
    mem.Write(0x0021, 0x80);     // High Byte -> Target Base: 0x8000
    mem.WriteROM(0x8000, 0x37);  // Target Value
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}