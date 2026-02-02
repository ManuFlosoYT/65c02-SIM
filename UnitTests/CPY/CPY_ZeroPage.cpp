#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CPY_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPY_ZeroPage_Test, CPY_ZeroPage) {
    // Y = 5, Mem = 5
    cpu.Y = 0x05;

    mem[0xFFFC] = INS_CPY_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
