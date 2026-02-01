#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class DEY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEY_Test, DEY) {
    // Y = 0x05 -> 0x04
    cpu.Y = 0x05;

    mem[0xFFFC] = INS_DEY_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEY_Test, DEY_ZeroFlag) {
    // Y = 0x01 -> 0x00
    cpu.Y = 0x01;

    mem[0xFFFC] = INS_DEY_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEY_Test, DEY_NegativeFlag) {
    // Y = 0x00 -> 0xFF
    cpu.Y = 0x00;

    mem[0xFFFC] = INS_DEY_A;
    mem[0xFFFD] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
