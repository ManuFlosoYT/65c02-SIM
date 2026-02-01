#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STX_ZeroPageY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STX_ZeroPageY_Test, STX_ZeroPageY) {
    cpu.X = 0x37;
    cpu.Y = 0x0F;

    // 0xFFFC: STX (ZeroPageY) 0x80
    mem[0xFFFC] = INS_STX_ZPY;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0xFF;  // Stop

    // Target Zero Page Address = 0x80 + 0x0F = 0x8F
    mem[0x008F] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x008F], 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
