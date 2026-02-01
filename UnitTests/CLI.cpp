#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class CLI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLI_Test, CLI_ClearsInterruptDisable) {
    cpu.I = 1;

    mem[0xFFFC] = INS_CLI;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.I);
}
