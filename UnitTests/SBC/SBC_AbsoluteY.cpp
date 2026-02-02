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

    mem[0xFFFC] = INS_SBC_ABSY;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x05;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}
