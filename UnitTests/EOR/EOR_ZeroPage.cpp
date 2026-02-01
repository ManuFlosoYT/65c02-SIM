#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_ZeroPage_Test, EOR_ZeroPage) {
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x0F;  // 0xFF ^ 0x0F = 0xF0
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
