#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class EOR_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_IndirectZP_Test, EOR_IndirectZP) {
    cpu.A = 0xFF;
    mem[0xFFFC] = INS_EOR_IND_ZP;
    mem[0xFFFD] = 0x20;  // ZP Pointer
    mem[0x0020] = 0x00;  // Low Byte
    mem[0x0021] = 0x80;  // High Byte -> Target Base: 0x8000
    mem[0x8000] = 0x0F;  // Target Value. 0xFF ^ 0x0F = 0xF0
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
