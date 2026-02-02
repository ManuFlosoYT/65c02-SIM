#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class SEC_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SEC_Test, SEC_SetsCarry) {
    cpu.C = 0;

    mem[0xFFFC] = INS_SEC;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.C);
}
