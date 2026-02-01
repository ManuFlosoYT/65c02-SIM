#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_ZeroPage_Test, ADC_ZeroPage) {
    // 0 + Mem[0x42](5) = 5
    cpu.A = 0x00;
    cpu.C = 0;

    mem[0xFFFC] = INS_ADC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
}
