#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class BIT_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(BIT_AbsoluteX_Test, BIT_AbsoluteX) {
    // Address = 0x4480 + 0x01 = 0x4481
    // Val = 0x80 (Bit 7 set -> N=1, V=0)
    cpu.X = 0x01;
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_BIT_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x80;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
    EXPECT_FALSE(cpu.V);
}
