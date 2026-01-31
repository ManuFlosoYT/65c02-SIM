#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDX_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY) {
    cpu.Y = 0x01;
    // 0xFFFC: LDX 0x8000, Y
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0x01 = 0x8001
    // 0x8001: 0x37
    mem[0xFFFC] = INS_LDX_ABSY;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8001] = 0x37;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x0000);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_PageCrossing) {
    cpu.Y = 0xFF;
    // 0xFFFC: LDX 0x8000, Y
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0xFF = 0x80FF
    // 0x80FF: 0x37
    mem[0xFFFC] = INS_LDX_ABSY;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x80FF] = 0x37;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;
    cpu.Y = 0x01;

    mem[0xFFFC] = INS_LDX_ABSY;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8001] = 0x00;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_AbsoluteY_Test, LDX_AbsoluteY_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;
    cpu.Y = 0x01;

    mem[0xFFFC] = INS_LDX_ABSY;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8001] = 0x80;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
