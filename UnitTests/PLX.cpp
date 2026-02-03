#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PLX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PLX_Test, PLX) {
    // 0xFFFC: PLX
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_PLX;
    mem[0x4001] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_EQ(cpu.SP, 0x01FF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLX_Test, PLX_ZeroFlag) {
    // 0xFFFC: PLX
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_PLX;
    mem[0x4001] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLX_Test, PLX_NegativeFlag) {
    // 0xFFFC: PLX
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_PLX;
    mem[0x4001] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x80;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}