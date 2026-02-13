#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class STZ_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_Absolute_Test, STZ_Absolute_ExecutesCorrectly) {
    cpu.PC = 0xFFFC;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STZ_ABS);
    mem.Write(0x4001, 0x00);  // Low
    mem.Write(0x4002, 0x20);  // High -> 0x2000
    mem.Write(0x4003, INS_JAM);  // Stop

    mem.Write(0x2000, 0xDD);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x2000], 0x00);
    EXPECT_EQ(cpu.PC, 0x4004);  // 16-bit wrap from FFFF
}