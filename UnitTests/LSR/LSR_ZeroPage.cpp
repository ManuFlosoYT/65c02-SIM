#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LSR_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LSR_ZeroPage_Test, LSR_ZeroPage) {
    // Mem[0x42] = 0x04 -> 0x02
    mem[0xFFFC] = INS_LSR_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x04;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x02);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LSR_ZeroPage_Test, LSR_ZeroPage_Carry) {
    // Mem[0x42] = 0x01 -> 0x00. C=1
    mem[0xFFFC] = INS_LSR_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x01;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
}
