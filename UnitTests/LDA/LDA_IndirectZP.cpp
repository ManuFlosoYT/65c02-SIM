#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDA_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP) {
    // 0xFFFC: LDA (ZP)
    // 0xFFFD: 0x20 (Pointer address in ZP)
    // 0x0020: 0x00 (Low byte of target)
    // 0x0021: 0x80 (High byte of target) -> Target Address: 0x8000
    // 0x8000: 0x37 (Value to load)

    mem[0xFFFC] = INS_LDA_IND_ZP;
    mem[0xFFFD] = 0x20;
    mem[0x0020] = 0x00;
    mem[0x0021] = 0x80;
    mem[0x8000] = 0x37;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_ZeroFlag) {
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_LDA_IND_ZP;
    mem[0xFFFD] = 0x20;
    mem[0x0020] = 0x00;
    mem[0x0021] = 0x80;
    mem[0x8000] = 0x00;  // Load 0x00
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_NegativeFlag) {
    cpu.A = 0x00;

    mem[0xFFFC] = INS_LDA_IND_ZP;
    mem[0xFFFD] = 0x20;
    mem[0x0020] = 0x00;
    mem[0x0021] = 0x80;
    mem[0x8000] = 0x80;  // Load 0x80 (Negative)
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_WrapAround) {
    // Test the ZP wrap-around behavior explicitly handled in LDA implementation
    // Pointer at 0xFF.
    // Low byte at 0xFF.
    // High byte at 0x00 (Wrap around).

    mem[0xFFFC] = INS_LDA_IND_ZP;
    mem[0xFFFD] = 0xFF;
    mem[0x00FF] = 0x00;  // Low byte of target address
    mem[0x0000] = 0x90;  // High byte of target address -> Target: 0x9000
    mem[0x9000] = 0x42;  // Value to load
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
