#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_IndirectX_Test, EOR_IndirectX) {
    cpu.X = 0x04;
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_INDX;
    mem[0xFFFD] = 0x02;  // ZP Addr = 0x02. Actual Lookup: 0x02 + 0x04 = 0x06
    mem[0x0006] = 0x00;  // Low Byte
    mem[0x0007] = 0x80;  // High Byte -> Eff Address: 0x8000
    mem[0x8000] = 0x0F;  // 0xFF ^ 0x0F = 0xF0
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
