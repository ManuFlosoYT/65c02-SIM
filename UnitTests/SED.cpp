#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class SED_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SED_Test, SED_SetsDecimal) {
    cpu.D = 0;

    mem[0xFFFC] = INS_SED;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.D);
}
