#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STZ_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_ExecutesCorrectly) {
    cpu.PC = 0x1000;
    cpu.X = 0x05;
    mem[0x1000] = INS_STZ_ABSX;
    mem[0x1001] = 0x00;
    mem[0x1002] = 0x20;     // 0x2000
    mem[0x1003] = INS_JAM;  // Stop

    // Target: 0x2000 + 0x05 = 0x2005
    mem[0x2005] = 0xEE;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x10;
    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2005], 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_PageCrossing) {
    cpu.PC = 0x1000;
    cpu.X = 0xFF;
    mem[0x1000] = INS_STZ_ABSX;
    mem[0x1001] = 0x00;
    mem[0x1002] = 0x20;  // 0x2000
    mem[0x1003] = INS_JAM;

    // Target: 0x2000 + 0xFF = 0x20FF
    mem[0x20FF] = 0xEE;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x10;
    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x20FF], 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}