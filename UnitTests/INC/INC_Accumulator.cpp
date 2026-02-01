#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class INC_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INC_Accumulator_Test, INC_Accumulator) {
    // A = 0x05 -> A + 1 = 0x06
    cpu.A = 0x05;

    mem[0xFFFC] = INS_INC_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_Accumulator_Test, INC_Accumulator_ZeroFlag) {
    // A = 0xFF -> A + 1 = 0x00
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_INC_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_Accumulator_Test, INC_Accumulator_NegativeFlag) {
    // A = 0x7F -> A + 1 = 0x80 (-128)
    cpu.A = 0x7F;

    mem[0xFFFC] = INS_INC_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
