#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CPX_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPX_ZeroPage_Test, CPX_ZeroPage) {
    // X = 5, Mem = 5
    cpu.X = 0x05;

    mem[0xFFFC] = INS_CPX_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
