#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ORA_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_ZeroPageX_Test, ORA_ZeroPageX) {
    cpu.X = 0x05;
    cpu.A = 0x00;
    mem[0xFFFC] = INS_ORA_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0047] = 0x0F;  // 0x42 + 0x05 = 0x47. 0x00 | 0x0F = 0x0F
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
