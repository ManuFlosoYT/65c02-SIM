#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ASL_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// ASL: Shift Left One Bit (Memory or Accumulator)
// C <- [7...0] <- 0

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_NoCarry) {
    // 0000 0001 (1) -> 0000 0010 (2). C=0
    cpu.A = 0x01;
    cpu.C = 1;  // Should be cleared

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ASL_A;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x02);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_CarryOut) {
    // 1000 0000 (0x80) -> 0000 0000 (0). C=1
    cpu.A = 0x80;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ASL_A;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_Negative) {
    // 0100 0000 (0x40) -> 1000 0000 (0x80). N=1
    cpu.A = 0x40;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ASL_A;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}