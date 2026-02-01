#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_ZeroPageX_Test, EOR_ZeroPageX) {
    cpu.X = 0x05;
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0047] = 0x0F;  // 0x42 + 0x05 = 0x47. 0xFF ^ 0x0F = 0xF0
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
