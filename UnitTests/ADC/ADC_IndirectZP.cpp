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

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_IND_ZP);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0005, 0x00);
    mem.Write(0x0006, 0x80);  // Pointer -> 0x8000
    mem.Write(0x8000, 0x10);  // Value

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

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_IND_ZP);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0005, 0x00);
    mem.Write(0x0006, 0x80);  // Pointer -> 0x8000
    mem.Write(0x8000, 0x10);  // Value

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x21);  // 0x10 + 0x10 + 1 = 0x21
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Overflow) {
    cpu.A = 0x50;  // Positive (+80)
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_IND_ZP);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0010, 0x00);
    mem.Write(0x0011, 0x90);  // Pointer -> 0x9000
    mem.Write(0x9000, 0x50);  // Positive (+80)

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

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_IND_ZP);
    mem.Write(0x4001, 0xFF);     // ZP Boundary
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x00FF, 0x10);     // Low
    mem.Write(0x0000, 0xC0);     // High (wrapped) -> 0xC010
    mem.Write(0xC010, 0xFE);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);  // 0x01 + 0xFE = 0xFF
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}