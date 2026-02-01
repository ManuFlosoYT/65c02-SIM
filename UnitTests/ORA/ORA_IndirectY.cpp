#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ORA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_IndirectY_Test, ORA_IndirectY) {
    cpu.Y = 0x04;
    cpu.A = 0x00;
    mem[0xFFFC] = INS_ORA_INDY;
    mem[0xFFFD] = 0x02;  // ZP
    mem[0x0002] = 0x00;  // Low Byte
    mem[0x0003] = 0x80;  // High Byte -> Base: 0x8000
    mem[0x8004] = 0x0F;  // 0x8000 + 0x04. 0x00 | 0x0F = 0x0F
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
