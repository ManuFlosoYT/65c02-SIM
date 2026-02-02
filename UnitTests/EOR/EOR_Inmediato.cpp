#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_Inmediato_Test, EOR_Inmediato) {
    cpu.A = 0xFF;  // Start with 0xFF
    mem[0xFFFC] = INS_EOR_IM;
    mem[0xFFFD] = 0x0F;  // 0xFF ^ 0x0F = 0xF0
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(EOR_Inmediato_Test, EOR_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_IM;
    mem[0xFFFD] = 0xFF;  // 0xFF ^ 0xFF = 0x00
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(EOR_Inmediato_Test, EOR_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0x00;
    mem[0xFFFC] = INS_EOR_IM;
    mem[0xFFFD] = 0x80;  // 0x00 ^ 0x80 = 0x80
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
