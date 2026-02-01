#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class TYA_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TYA_Test, TYA) {
    // 0xFFFC: TYA
    mem[0xFFFC] = INS_TYA;
    mem[0xFFFD] = 0xFF;  // Stop

    cpu.Y = 0x42;
    cpu.A = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_ZeroFlag) {
    // 0xFFFC: TYA
    mem[0xFFFC] = INS_TYA;
    mem[0xFFFD] = 0xFF;  // Stop

    cpu.Y = 0x00;
    cpu.A = 0x42;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_NegativeFlag) {
    // 0xFFFC: TYA
    mem[0xFFFC] = INS_TYA;
    mem[0xFFFD] = 0xFF;  // Stop

    cpu.Y = 0x80;
    cpu.A = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
