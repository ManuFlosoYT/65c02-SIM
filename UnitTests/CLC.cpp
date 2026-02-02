#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class CLC_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLC_Test, CLC_ClearsCarry) {
    cpu.C = 1;

    mem[0xFFFC] = INS_CLC;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.C);
}
