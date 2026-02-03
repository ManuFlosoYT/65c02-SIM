#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class TAY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TAY_Test, TAY) {
    // 0xFFFC: TAY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x42;
    cpu.Y = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TAY_Test, TAY_ZeroFlag) {
    // 0xFFFC: TAY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x00;
    cpu.Y = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TAY_Test, TAY_NegativeFlag) {
    // 0xFFFC: TAY
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TAY);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x80;
    cpu.Y = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}