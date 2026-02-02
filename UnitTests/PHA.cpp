#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PHA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PHA_Test, PHA) {
    // 0xFFFC: PHA
    mem[0xFFFC] = INS_PHA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.A = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x01FF], 0x42);
    EXPECT_EQ(cpu.SP, 0x01FE);
}
