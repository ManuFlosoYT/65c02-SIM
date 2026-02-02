#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CMP_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_ZeroPageX_Test, CMP_ZeroPageX) {
    // A = 5, Mem = 5
    cpu.A = 0x05;
    cpu.X = 0x01;

    mem[0xFFFC] = INS_CMP_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0043] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
