#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class ORA_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_IndirectZP_Test, ORA_IndirectZP) {
    cpu.A = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_IND_ZP);
    mem.Write(0x4001, 0x20);  // ZP Pointer
    mem.Write(0x0020, 0x00);  // Low Byte
    mem.Write(0x0021, 0x80);  // High Byte -> Target Base: 0x8000
    mem.Write(0x8000, 0x0F);  // Target Value.
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}