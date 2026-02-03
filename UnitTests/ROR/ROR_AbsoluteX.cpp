#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROR_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROR_AbsoluteX_Test, ROR_AbsoluteX) {
    // Mem = 0x01 -> 0x80. C=1.
    cpu.X = 0x01;
    cpu.C = 1;  // carry in ignored

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ROR_ABSX;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x01;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4481], 0x80);
    EXPECT_TRUE(cpu.C);
}