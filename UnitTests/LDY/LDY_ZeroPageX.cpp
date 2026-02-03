#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDY_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDY_ZeroPageX_Test, LDY_ZeroPageX) {
    cpu.X = 0x04;
    // 0xFFFC: LDY 0x42, X
    // 0xFFFD: 0x42
    // Address: 0x42 + 0x04 = 0x46
    // 0x0046: 0x37
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0046, 0x37);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_ZeroPageX_Test, LDY_ZeroPageX_WrapAround) {
    cpu.X = 0xFF;
    // 0xFFFC: LDY 0x80, X
    // 0xFFFD: 0x80
    // Address: (0x80 + 0xFF) & 0xFF = 0x7F
    // 0x007F: 0x37
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x007F, 0x37);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(LDY_ZeroPageX_Test, LDY_ZeroPageX_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;
    cpu.X = 0x0A;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x004C, 0x00);  // 0x42 + 0x0A
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_ZeroPageX_Test, LDY_ZeroPageX_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;
    cpu.X = 0x0A;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x004C, 0x80);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}