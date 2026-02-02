#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROL_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// ROL: Rotate Left Circular (8-bit)
// [7...0] <- [6...0, 7]
// C <- Old Bit 7

TEST_F(ROL_Accumulator_Test, ROL_Accumulator_Rotate) {
    // 1000 0000 (0x80) -> 0000 0001 (0x01). Bit 7 wraps to 0.
    // Carry Out = Old Bit 7 (1).
    cpu.A = 0x80;
    cpu.C = 0;  // Carry In should be ignored

    mem[0xFFFC] = INS_ROL_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ROL_Accumulator_Test, ROL_Accumulator_NoCarryOut) {
    // 0100 0000 (0x40) -> 1000 0000 (0x80).
    // Carry Out = Old Bit 7 (0).
    cpu.A = 0x40;
    cpu.C = 1;  // Carry In should be ignored

    mem[0xFFFC] = INS_ROL_A;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
