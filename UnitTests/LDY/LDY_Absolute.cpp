#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDY_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDY_Absolute_Test, LDY_Absolute) {
    // 0xFFFC: LDY 0x8000
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // 0x8000: 0x37
    mem[0xFFFC] = INS_LDY_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x37;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x0000);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Absolute_Test, LDY_Absolute_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;

    mem[0xFFFC] = INS_LDY_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x00;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Absolute_Test, LDY_Absolute_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;

    mem[0xFFFC] = INS_LDY_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x80;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
