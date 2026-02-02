#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class TXA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TXA_Test, TXA) {
    // 0xFFFC: TXA
    mem[0xFFFC] = INS_TXA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.X = 0x42;
    cpu.A = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TXA_Test, TXA_ZeroFlag) {
    // 0xFFFC: TXA
    mem[0xFFFC] = INS_TXA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.X = 0x00;
    cpu.A = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TXA_Test, TXA_NegativeFlag) {
    // 0xFFFC: TXA
    mem[0xFFFC] = INS_TXA;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.X = 0x80;
    cpu.A = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
