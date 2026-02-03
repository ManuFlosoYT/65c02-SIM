#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_IndirectY_Test, ADC_IndirectY) {
    // ZP Addr = 0x02
    // Mem[0x02] = 0x00, Mem[0x03] = 0x80 -> Base Addr = 0x8000
    // Addr = 0x8000 + 0x01 = 0x8001
    // Mem[0x8001] = 0x37
    cpu.Y = 0x01;
    cpu.A = 0x00;
    cpu.C = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ADC_INDY;
    mem[0x4001] = 0x02;
    mem[0x0002] = 0x00;
    mem[0x0003] = 0x80;
    mem[0x8001] = 0x37;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
}