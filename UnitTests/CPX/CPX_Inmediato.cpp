#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CPX_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPX_Inmediato_Test, CPX_Inmediato) {
    // X = 5, M = 5. Result = 0. Z=1, C=1.
    cpu.X = 0x05;

    mem[0xFFFC] = INS_CPX_IM;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CPX_Inmediato_Test, CPX_Inmediato_Less) {
    // X = 4, M = 5. Result = 0xFF. Z=0, C=0. N=1.
    cpu.X = 0x04;

    mem[0xFFFC] = INS_CPX_IM;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CPX_Inmediato_Test, CPX_Inmediato_Greater) {
    // X = 6, M = 5. Result = 1. Z=0, C=1. N=0.
    cpu.X = 0x06;

    mem[0xFFFC] = INS_CPX_IM;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}
