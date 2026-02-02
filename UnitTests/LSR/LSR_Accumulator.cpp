#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LSR_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// LSR: Logical Shift Right
// 0 -> [7...0] -> C

TEST_F(LSR_Accumulator_Test, LSR_Accumulator_NoCarry) {
    // 0000 0010 (2) -> 0000 0001 (1). C=0
    cpu.A = 0x02;
    cpu.C = 1;

    mem[0xFFFC] = INS_LSR_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LSR_Accumulator_Test, LSR_Accumulator_CarryOut) {
    // 0000 0001 (1) -> 0000 0000 (0). C=1
    cpu.A = 0x01;

    mem[0xFFFC] = INS_LSR_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);  // LSR always resets N
}
