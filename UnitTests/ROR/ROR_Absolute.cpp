#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROR_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROR_Absolute_Test, ROR_Absolute) {
    // Mem = 0x01 -> 0x00 (Shift Right, C=0 into bit 7).
    // Carry Out = Old Bit 0 (1).
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ROR_ABS;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x20;  // 0x2080
    mem[0x2080] = 0x01;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2080], 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}