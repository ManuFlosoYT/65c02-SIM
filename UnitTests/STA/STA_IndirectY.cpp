#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_IndirectY_Test, STA_IndirectY) {
    cpu.A = 0x37;
    cpu.Y = 0x10;

    // 0xFFFC: STA (IndirectY) 0x20
    mem[0xFFFC] = INS_STA_INDY;
    mem[0xFFFD] = 0x20;  // Zero Page
    mem[0xFFFE] = INS_JAM;  // Stop

    // Pointer at 0x20 (ZP) -> 0x2070
    mem[0x0020] = 0x70;  // Low
    mem[0x0021] = 0x20;  // High

    // Effective Address = 0x2070 + 0x10 = 0x2080
    mem[0x2080] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2080], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
