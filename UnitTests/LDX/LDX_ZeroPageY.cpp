#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDX_ZeroPageY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY) {
    cpu.Y = 0x04;
    // 0xFFFC: LDX 0x42, Y
    // 0xFFFD: 0x42
    // Address: 0x42 + 0x04 = 0x46
    // 0x0046: 0x37
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDX_ZPY;
    mem[0x4001] = 0x42;
    mem[0x0046] = 0x37;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_WrapAround) {
    cpu.Y = 0xFF;
    // 0xFFFC: LDX 0x80, Y
    // 0xFFFD: 0x80
    // Address: (0x80 + 0xFF) & 0xFF = 0x7F
    // 0x007F: 0x37
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDX_ZPY;
    mem[0x4001] = 0x80;
    mem[0x007F] = 0x37;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;
    cpu.Y = 0x0A;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDX_ZPY;
    mem[0x4001] = 0x42;
    mem[0x004C] = 0x00;  // 0x42 + 0x0A
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;
    cpu.Y = 0x0A;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDX_ZPY;
    mem[0x4001] = 0x42;
    mem[0x004C] = 0x80;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}