#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CMP_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_AbsoluteY_Test, CMP_AbsoluteY) {
    cpu.A = 0x05;
    cpu.Y = 0x01;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_CMP_ABSY;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x05;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}