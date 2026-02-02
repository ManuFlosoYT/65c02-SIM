#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROR_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROR_ZeroPage_Test, ROR_ZeroPage) {
    // Mem = 0x01 -> 0x80. C=1.
    cpu.C = 0;

    mem[0xFFFC] = INS_ROR_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x01;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x80);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.N);
}
