#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_AbsoluteX_Test, EOR_AbsoluteX) {
    cpu.X = 0x01;
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;  // 0x4480
    mem[0x4481] = 0x0F;  // 0x4480 + 0x01. 0xFF ^ 0x0F = 0xF0
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
