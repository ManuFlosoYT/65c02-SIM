#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROR_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROR_ZeroPageX_Test, ROR_ZeroPageX) {
    // Mem = 0x01 -> 0x80. C=1.
    cpu.X = 0x02;
    cpu.C = 0;

    mem[0xFFFC] = INS_ROR_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0044] = 0x01;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0044], 0x80);
    EXPECT_TRUE(cpu.C);
}
