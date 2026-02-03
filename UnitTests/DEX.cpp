#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class DEX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEX_Test, DEX) {
    // X = 0x05 -> 0x04
    cpu.X = 0x05;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_DEX;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEX_Test, DEX_ZeroFlag) {
    // X = 0x01 -> 0x00
    cpu.X = 0x01;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_DEX;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEX_Test, DEX_NegativeFlag) {
    // X = 0x00 -> 0xFF
    cpu.X = 0x00;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_DEX;
    mem[0x4001] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}