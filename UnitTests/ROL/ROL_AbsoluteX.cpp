#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROL_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROL_AbsoluteX_Test, ROL_AbsoluteX) {
    // Mem = 0x80 -> 0x01. C=1.
    cpu.X = 0x01;
    cpu.C = 0;

    mem[0xFFFC] = INS_ROL_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x80;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4481], 0x01);
    EXPECT_TRUE(cpu.C);
}
