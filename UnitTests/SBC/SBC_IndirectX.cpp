#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_IndirectX_Test, SBC_IndirectX) {
    cpu.X = 0x04;
    cpu.A = 0x0A;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_INDX;
    mem[0xFFFD] = 0x02;
    mem[0x0006] = 0x00;
    mem[0x0007] = 0x80;
    mem[0x8000] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}
