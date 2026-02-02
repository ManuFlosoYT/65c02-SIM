#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CMP_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Equal) {
    cpu.A = 0x10;
    cpu.C = 0;  // Pre-condition check

    mem[0xFFFC] = INS_CMP_IND_ZP;
    mem[0xFFFD] = 0x05;     // ZP Addr
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0005] = 0x00;
    mem[0x0006] = 0x80;  // Pointer -> 0x8000
    mem[0x8000] = 0x10;  // Match A

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Greater) {
    cpu.A = 0x20;

    mem[0xFFFC] = INS_CMP_IND_ZP;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0010] = 0x00;
    mem[0x0011] = 0x90;  // Pointer -> 0x9000
    mem[0x9000] = 0x10;  // A > M (0x20 > 0x10)

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x20);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Less) {
    cpu.A = 0x10;

    mem[0xFFFC] = INS_CMP_IND_ZP;
    mem[0xFFFD] = 0x20;
    mem[0xFFFE] = INS_JAM;  // Stop
    mem[0x0020] = 0x00;
    mem[0x0021] = 0xA0;  // Pointer -> 0xA000
    mem[0xA000] = 0x20;  // A < M (0x10 < 0x20)

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);  // A < M
    EXPECT_TRUE(cpu.N);   // Negative result (approx 16 - 32)
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x50;

    mem[0xFFFC] = INS_CMP_IND_ZP;
    mem[0xFFFD] = 0xFF;  // ZP Boundary
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x00FF] = 0x10;  // Low
    mem[0x0000] = 0xB0;  // High (wrapped) -> 0xB010
    mem[0xB010] = 0x50;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x50);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);
}
