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

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEY);
    mem.Write(0x4001, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEY_Test, DEY_ZeroFlag) {
    // Y = 0x01 -> 0x00
    cpu.Y = 0x01;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEY);
    mem.Write(0x4001, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEY_Test, DEY_NegativeFlag) {
    // Y = 0x00 -> 0xFF
    cpu.Y = 0x00;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEY);
    mem.Write(0x4001, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}