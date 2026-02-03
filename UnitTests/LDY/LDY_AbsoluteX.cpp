#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDY_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX) {
    cpu.X = 0x01;
    // 0xFFFC: LDY 0x8000, X
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0x01 = 0x8001
    // 0x8001: 0x37
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_ABSX;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8001] = 0x37;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_PageCrossing) {
    cpu.X = 0xFF;
    // 0xFFFC: LDY 0x8000, X
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0xFF = 0x80FF
    // 0x80FF: 0x37
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_ABSX;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x80FF] = 0x37;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;
    cpu.X = 0x01;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_ABSX;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8001] = 0x00;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;
    cpu.X = 0x01;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDY_ABSX;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;
    mem[0x8001] = 0x80;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}