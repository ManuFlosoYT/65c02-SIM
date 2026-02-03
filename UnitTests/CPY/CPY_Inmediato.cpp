#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CPY_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPY_Inmediato_Test, CPY_Inmediato) {
    // Y = 5, M = 5. Result = 0. Z=1, C=1.
    cpu.Y = 0x05;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_CPY_IM;
    mem[0x4001] = 0x05;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CPY_Inmediato_Test, CPY_Inmediato_Less) {
    // Y = 4, M = 5. Result = 0xFF. Z=0, C=0. N=1.
    cpu.Y = 0x04;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_CPY_IM;
    mem[0x4001] = 0x05;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CPY_Inmediato_Test, CPY_Inmediato_Greater) {
    // Y = 6, M = 5. Result = 1. Z=0, C=1. N=0.
    cpu.Y = 0x06;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_CPY_IM;
    mem[0x4001] = 0x05;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}