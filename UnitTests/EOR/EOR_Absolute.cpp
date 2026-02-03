#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_Absolute_Test, EOR_Absolute) {
    cpu.A = 0xFF;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_EOR_ABS;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;  // 0x4480
    mem[0x4480] = 0x0F;  // 0xFF ^ 0x0F = 0xF0
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}