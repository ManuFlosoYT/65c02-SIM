#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class ROR_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROR_ZeroPage_Test, ROR_ZeroPage) {
    // Mem = 0x01 -> 0x00 (Shift Right, C=0 into bit 7).
    // Carry Out = Old Bit 0 (1).
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ROR_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x01);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}