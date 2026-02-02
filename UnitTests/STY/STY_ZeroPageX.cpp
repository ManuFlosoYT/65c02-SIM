#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STY_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STY_ZeroPageX_Test, STY_ZeroPageX) {
    cpu.Y = 0x37;
    cpu.X = 0x0F;

    // 0xFFFC: STY (ZeroPageX) 0x80
    mem[0xFFFC] = INS_STY_ZPX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = INS_JAM;  // Stop

    // Target Zero Page Address = 0x80 + 0x0F = 0x8F
    mem[0x008F] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x008F], 0x37);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
