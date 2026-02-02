#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class BIT_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(BIT_ZeroPageX_Test, BIT_ZeroPageX) {
    // Address = 0x42 + 0x05 = 0x47
    // Val at 0x47 = 0xC0 (N=1, V=1)
    cpu.X = 0x05;
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_BIT_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0047] = 0xC0;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
}
