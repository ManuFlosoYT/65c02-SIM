#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ADC_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_AbsoluteX_Test, ADC_AbsoluteX) {
    // Addr = 0x4480 + 1 = 0x4481
    // 0 + Mem[0x4481](5) = 5
    cpu.A = 0x00;
    cpu.X = 0x01;
    cpu.C = 0;

    mem[0xFFFC] = INS_ADC_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x05;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}
