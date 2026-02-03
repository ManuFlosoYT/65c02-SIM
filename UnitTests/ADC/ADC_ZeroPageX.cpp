#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_ZeroPageX_Test, ADC_ZeroPageX) {
    // Addr = 0x42 + 0x01 = 0x43
    // 0 + Mem[0x43](5) = 5
    cpu.A = 0x00;
    cpu.X = 0x01;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_ZPX;
    mem[0x4001] = 0x42;
    mem[0x0043] = 0x05;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}