#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class TRB_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TRB_Absolute_Test, TRB_Absolute_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = M & ~A = 0x55 & 0x55 = 0x55
    cpu.A = 0xAA;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_TRB_ABS;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8000] = 0x55;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0x55);
}

TEST_F(TRB_Absolute_Test, TRB_Absolute_ClearsZeroFlag_And_ResetsBits) {
    // A = 0xFF (1111 1111)
    // M = 0xFF (1111 1111)
    // A & M = 0xFF != 0 -> Z = 0
    // M = M & ~A = 0xFF & 0x00 = 0x00
    cpu.A = 0xFF;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_TRB_ABS;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8000] = 0xFF;
    mem[0x4003] = INS_JAM;
    // -> 8000 (Exec?) No.
    // 0xFFFC: Op
    // 0xFFFD: Lo
    // 0xFFFE: Hi
    // 0xFFFF: Next Opcode

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0x00);
}

TEST_F(TRB_Absolute_Test, TRB_Absolute_PartialReset) {
    // A = 0xF0 (1111 0000)
    // M = 0xAA (1010 1010)
    // A & M = 0xA0 (1010 0000) != 0 -> Z = 0
    // M = M & ~A = 0xAA & 0x0F = 0x0A (0000 1010)
    cpu.A = 0xF0;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_TRB_ABS;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8000] = 0xAA;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0x0A);
}