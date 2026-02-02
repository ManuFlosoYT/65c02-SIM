#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROR_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// ROR: Rotate Right Circular (8-bit)
// [7...0] <- [0, 7...1]
// C <- Old Bit 0

TEST_F(ROR_Accumulator_Test, ROR_Accumulator_Rotate) {
    // 0000 0001 (0x01) -> 1000 0000 (0x80). Bit 0 wraps to 7.
    // Carry Out = Old Bit 0 (1).
    cpu.A = 0x01;
    cpu.C = 0;  // Carry In ignored

    mem[0xFFFC] = INS_ROR_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(ROR_Accumulator_Test, ROR_Accumulator_NoCarryOut) {
    // 0000 0010 (0x02) -> 0000 0001 (0x01).
    // Carry Out = Old Bit 0 (0).
    cpu.A = 0x02;
    cpu.C = 1;  // Carry In ignored

    mem[0xFFFC] = INS_ROR_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
