#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// SBC: A = A - M - (1 - C)
// Note: 6502 subtraction requires C=1 for "no borrow".

TEST_F(SBC_Inmediato_Test, SBC_Inmediato) {
    // 5 - 3 = 2. C=1 (no borrow)
    // A=5, M=3, C=1. Result = 5 - 3 - 0 = 2.
    cpu.A = 0x05;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_IM;
    mem[0xFFFD] = 0x03;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x02);
    EXPECT_TRUE(cpu.C);  // No borrow occurred, so C remains 1
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(SBC_Inmediato_Test, SBC_Inmediato_Borrow) {
    // 5 - 6 = -1 (0xFF). C=1 (no borrow start).
    // A=5, M=6, C=1. Result = 5 - 6 - 0 = -1 (0xFF).
    cpu.A = 0x05;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_IM;
    mem[0xFFFD] = 0x06;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.C);  // Borrow occurred, C=0
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(SBC_Inmediato_Test, SBC_Inmediato_BorrowIn) {
    // 5 - 3 - 1 = 1. C=0 (borrow incoming)
    // A=5, M=3, C=0. Result = 5 - 3 - 1 = 1.
    cpu.A = 0x05;
    cpu.C = 0;

    mem[0xFFFC] = INS_SBC_IM;
    mem[0xFFFD] = 0x03;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);  // No borrow occurred for this operation, C=1
}
