#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class INC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INC_ZeroPage_Test, INC_ZeroPage) {
    // Mem[0x42] = 0x05 -> 0x06
    mem[0xFFFC] = INS_INC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_ZeroFlag) {
    // Mem[0x42] = 0xFF -> 0x00
    mem[0xFFFC] = INS_INC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0xFF;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_NegativeFlag) {
    // Mem[0x42] = 0x7F -> 0x80
    mem[0xFFFC] = INS_INC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x7F;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
