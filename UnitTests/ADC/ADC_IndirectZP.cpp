#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Basic) {
    cpu.A = 0x10;
    cpu.C = 0;

    mem[0xFFFC] = INS_ADC_IND_ZP;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x0005] = 0x00;
    mem[0x0006] = 0x80;  // Pointer -> 0x8000
    mem[0x8000] = 0x10;  // Value

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x20);  // 0x10 + 0x10 + 0 = 0x20
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Carry) {
    cpu.A = 0x10;
    cpu.C = 1;

    mem[0xFFFC] = INS_ADC_IND_ZP;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x0005] = 0x00;
    mem[0x0006] = 0x80;  // Pointer -> 0x8000
    mem[0x8000] = 0x10;  // Value

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x21);  // 0x10 + 0x10 + 1 = 0x21
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Overflow) {
    cpu.A = 0x50;  // Positive (+80)
    cpu.C = 0;

    mem[0xFFFC] = INS_ADC_IND_ZP;
    mem[0xFFFD] = 0x10;
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x0010] = 0x00;
    mem[0x0011] = 0x90;  // Pointer -> 0x9000
    mem[0x9000] = 0x50;  // Positive (+80)

    // 0x50 + 0x50 = 0xA0 (-96) -> Overflow
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xA0);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);  // Overflow set
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x01;
    cpu.C = 0;

    mem[0xFFFC] = INS_ADC_IND_ZP;
    mem[0xFFFD] = 0xFF;  // ZP Boundary
    mem[0xFFFE] = 0xFF;  // Stop
    mem[0x00FF] = 0x10;  // Low
    mem[0x0000] = 0xC0;  // High (wrapped) -> 0xC010
    mem[0xC010] = 0xFE;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);  // 0x01 + 0xFE = 0xFF
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}
