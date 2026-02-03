#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STA_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_AbsoluteY_Test, STA_AbsoluteY) {
    cpu.A = 0x37;
    cpu.Y = 0x10;

    // 0xFFFC: STA (AbsoluteY) 0x2000
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_STA_ABSY;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x20;
    mem[0x4003] = INS_JAM;  // Stop

    // Target = 0x2000 + 0x10 = 0x2010
    mem[0x2010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2010], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}