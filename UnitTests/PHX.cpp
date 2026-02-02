#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PHX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PHX_Test, PHX) {
    // 0xFFFC: PHX
    mem[0xFFFC] = INS_PHX;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.X = 0x42;
    cpu.SP = 0x01FF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x01FF], 0x42);
    EXPECT_EQ(cpu.SP, 0x01FE);
}
