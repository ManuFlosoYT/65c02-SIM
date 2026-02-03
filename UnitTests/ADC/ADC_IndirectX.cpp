#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_IndirectX_Test, ADC_IndirectX) {
    // ZP Addr = 0x02 + 0x04 = 0x06
    // Mem[0x06] = 0x00, Mem[0x07] = 0x80 -> Addr = 0x8000
    // Mem[0x8000] = 0x37
    cpu.X = 0x04;
    cpu.A = 0x00;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_INDX;
    mem[0x4001] = 0x02;
    mem[0x0006] = 0x00;
    mem[0x0007] = 0x80;
    mem[0x8000] = 0x37;  // 55
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
}