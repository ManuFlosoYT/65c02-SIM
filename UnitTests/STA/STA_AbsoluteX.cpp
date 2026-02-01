#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STA_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_AbsoluteX_Test, STA_AbsoluteX) {
    cpu.A = 0x37;
    cpu.X = 0x10;

    // 0xFFFC: STA (AbsoluteX) 0x2000
    mem[0xFFFC] = INS_STA_ABSX;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x20;
    mem[0xFFFF] = 0xFF;  // Stop

    // Target = 0x2000 + 0x10 = 0x2010
    mem[0x2010] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2010], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
