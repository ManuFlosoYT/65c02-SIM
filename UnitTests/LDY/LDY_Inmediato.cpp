#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDY_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDY_Inmediato_Test, LDY_Inmediato) {
    // 0xFFFC: LDY #0x42
    // 0xFFFD: 0x42
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_IM;
    mem[0x4001] = 0x42;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.Y, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Inmediato_Test, LDY_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_IM;
    mem[0x4001] = 0x00;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Inmediato_Test, LDY_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_IM;
    mem[0x4001] = 0x80;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}