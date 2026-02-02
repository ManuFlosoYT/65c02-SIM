#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROL_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROL_ZeroPage_Test, ROL_ZeroPage) {
    // Mem = 0x80 -> 0x01. C=1.
    cpu.C = 0;

    mem[0xFFFC] = INS_ROL_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x80;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x01);
    EXPECT_TRUE(cpu.C);
}
