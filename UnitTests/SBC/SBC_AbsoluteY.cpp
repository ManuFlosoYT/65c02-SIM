#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_AbsoluteY_Test, SBC_AbsoluteY) {
    cpu.A = 0x0A;
    cpu.Y = 0x01;
    cpu.C = 1;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_SBC_ABSY;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x05;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}