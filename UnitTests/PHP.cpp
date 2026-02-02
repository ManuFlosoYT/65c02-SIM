#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PHP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PHP_Test, PHP) {
    // 0xFFFC: PHP
    mem[0xFFFC] = INS_PHP;
    mem[0xFFFD] = INS_JAM;  // Stop

    // Set some flags
    cpu.C = 1;
    cpu.Z = 1;
    cpu.N = 1;
    cpu.V = 1;
    cpu.D = 1;
    cpu.I = 1;
    cpu.B = 1; 

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x01FF], 0xDF);
    EXPECT_EQ(cpu.SP, 0x01FE);
}
