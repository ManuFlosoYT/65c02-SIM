#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_IndirectZP_Test, AND_IndirectZP) {
    // INS_AND_IND_ZP
    // Operand: Byte (ZP Address of Pointer)
    // Effective Address = Mem[ZP] | (Mem[ZP + 1] << 8)
    // No indexing
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_AND_IND_ZP;
    mem[0xFFFD] = 0x20;  // ZP Pointer
    mem[0x0020] = 0x00;  // Low Byte
    mem[0x0021] = 0x80;  // High Byte -> Target Base: 0x8000
    mem[0x8000] = 0x37;  // Target Value
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
