#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CMP_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_IndirectY_Test, CMP_IndirectY) {
    cpu.Y = 0x01;
    cpu.A = 0x05;

    mem[0xFFFC] = INS_CMP_INDY;
    mem[0xFFFD] = 0x02;
    mem[0x0002] = 0x00;
    mem[0x0003] = 0x80;
    mem[0x8001] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
