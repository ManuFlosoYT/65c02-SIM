#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class DEC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage) {
    // Mem[0x42] = 0x05 -> 0x04
    mem[0xFFFC] = INS_DEC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage_ZeroFlag) {
    // Mem[0x42] = 0x01 -> 0x00
    mem[0xFFFC] = INS_DEC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x01;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPage_Test, DEC_ZeroPage_NegativeFlag) {
    // Mem[0x42] = 0x00 -> 0xFF
    mem[0xFFFC] = INS_DEC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x00;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
