#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class CLD_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLD_Test, CLD_ClearsDecimal) {
    cpu.D = 1;

    mem[0xFFFC] = INS_CLD;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.D);
}
