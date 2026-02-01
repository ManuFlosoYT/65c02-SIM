#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STX_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STX_ZeroPage_Test, STX_ZeroPage) {
    cpu.X = 0x37;

    // 0xFFFC: STX (ZeroPage) 0x42
    mem[0xFFFC] = INS_STX_ZP;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0xFF;  // Stop

    mem[0x0042] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
