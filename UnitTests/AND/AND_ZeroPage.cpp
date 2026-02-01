#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_ZeroPage_Test, AND_ZeroPage) {
    // 0xFFFC: AND (ZP) 0x42
    // 0x0042: 0x37
    cpu.A = 0xFF;  // Start with A = 0xFF
    mem[0xFFFC] = INS_AND_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x37;  // Value to AND with
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);  // 0xFF & 0x37 = 0x37
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_ZeroPage_Test, AND_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xF0;

    mem[0xFFFC] = INS_AND_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x0F;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_ZeroPage_Test, AND_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_AND_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x80;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
