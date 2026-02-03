#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_Decimal_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_Decimal_Test, ADC_Decimal_Simple) {
    // 0x05 + 0x05 = 0x10 (BCD)
    cpu.A = 0x05;
    cpu.D = 1;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_IM;
    mem[0x4001] = 0x05;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(ADC_Decimal_Test, ADC_Decimal_Carry) {
    // 0x50 + 0x51 = 0x01 (BCD), C=1 (Overflows 100)
    cpu.A = 0x50;
    cpu.D = 1;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_IM;
    mem[0x4001] = 0x51;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(ADC_Decimal_Test, ADC_Decimal_CarryIn) {
    // 0x09 + 0x01 + C(1) = 0x11
    cpu.A = 0x09;
    cpu.D = 1;
    cpu.C = 1;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_IM;
    mem[0x4001] = 0x01;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x11);  // 9 + 1 + 1 = 11 (BCD 11 is 0x11)
    EXPECT_FALSE(cpu.C);
}