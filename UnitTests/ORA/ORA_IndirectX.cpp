#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ORA_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_IndirectX_Test, ORA_IndirectX) {
    cpu.X = 0x04;
    cpu.A = 0x00;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_ORA_INDX;
    mem[0x4001] = 0x02;  // ZP Addr = 0x02. Actual Lookup: 0x02 + 0x04 = 0x06
    mem[0x0006] = 0x00;  // Low Byte
    mem[0x0007] = 0x80;  // High Byte -> Eff Address: 0x8000
    mem[0x8000] = 0x0F;  // 0x00 | 0x0F = 0x0F
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}