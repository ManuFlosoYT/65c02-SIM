#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class SEI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SEI_Test, SEI_SetsInterruptDisable) {
    cpu.I = 0;

    mem[0xFFFC] = INS_SEI;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.I);
}
