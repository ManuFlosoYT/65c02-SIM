#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PLA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PLA_Test, PLA) {
    // 0xFFFC: PLA
    mem[0xFFFC] = INS_PLA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_EQ(cpu.SP, 0x01FF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLA_Test, PLA_ZeroFlag) {
    // 0xFFFC: PLA
    mem[0xFFFC] = INS_PLA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLA_Test, PLA_NegativeFlag) {
    // 0xFFFC: PLA
    mem[0xFFFC] = INS_PLA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.SP = 0x01FE;
    mem[0x01FF] = 0x80;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
