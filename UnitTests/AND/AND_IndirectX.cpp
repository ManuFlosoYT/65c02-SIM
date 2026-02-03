#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_IndirectX_Test, AND_IndirectX) {
    // INS_AND_INDX
    // Operand: Byte (ZP Address)
    // Effective Address = Mem[ZP + X] | (Mem[ZP + X + 1] << 8)
    cpu.X = 0x04;
    cpu.A = 0xFF;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_AND_INDX;
    mem[0x4001] = 0x02;  // ZP Addr = 0x02. Actual Lookup: 0x02 + 0x04 = 0x06
    mem[0x0006] = 0x00;  // Low Byte
    mem[0x0007] = 0x80;  // High Byte -> Eff Address: 0x8000
    mem[0x8000] = 0x37;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}