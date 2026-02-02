#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PHY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PHY_Test, PHY) {
    // 0xFFFC: PHY
    mem[0xFFFC] = INS_PHY;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.Y = 0x42;
    cpu.SP = 0x01FF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x01FF], 0x42);
    EXPECT_EQ(cpu.SP, 0x01FE);
}
