#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class BIT_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(BIT_Inmediato_Test, BIT_Inmediato_SetsZeroFlag) {
    // A = 0x0F, Val = 0xF0 -> A & Val = 0x00 -> Z=1
    cpu.A = 0x0F;
    cpu.Z = 0;
    cpu.N = 0;  // Should remain 0
    cpu.V = 0;  // Should remain 0

    mem[0xFFFC] = INS_BIT_IM;
    mem[0xFFFD] = 0xF0;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);  // A Not modified
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}

TEST_F(BIT_Inmediato_Test, BIT_Inmediato_ClearsZeroFlag) {
    // A = 0xFF, Val = 0x0F -> A & Val = 0x0F != 0 -> Z=0
    cpu.A = 0xFF;
    cpu.Z = 1;

    mem[0xFFFC] = INS_BIT_IM;
    mem[0xFFFD] = 0x0F;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(BIT_Inmediato_Test, BIT_Inmediato_DoesNotAffectNV) {
    // Immediate mode BIT does not copy bits 7 and 6 to N and V.
    // Operand = 0xC0 (Bits 7 and 6 set)
    cpu.A = 0xFF;
    cpu.N = 0;
    cpu.V = 0;

    mem[0xFFFC] = INS_BIT_IM;
    mem[0xFFFD] = 0xC0;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}
