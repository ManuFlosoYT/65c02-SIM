#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROL_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROL_ZeroPageX_Test, ROL_ZeroPageX) {
    // Mem = 0x80 -> 0x01. C=1.
    cpu.X = 0x02;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ROL_ZPX;
    mem[0x4001] = 0x42;
    mem[0x0044] = 0x80;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0044], 0x01);
    EXPECT_TRUE(cpu.C);
}