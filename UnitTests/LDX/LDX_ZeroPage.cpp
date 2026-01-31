#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDX_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage) {
    // 0xFFFC: LDX 0x42
    // 0xFFFD: 0x42
    // 0x0042: 0x37
    mem[0xFFFC] = INS_LDX_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x37;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFF);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;

    mem[0xFFFC] = INS_LDX_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x00;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPage_Test, LDX_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;

    mem[0xFFFC] = INS_LDX_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x80;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
