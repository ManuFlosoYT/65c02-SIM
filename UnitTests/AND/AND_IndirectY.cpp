#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_IndirectY_Test, AND_IndirectY) {
    // INS_AND_INDY
    // Operand: Byte (ZP Address)
    // Base Address = Mem[ZP] | (Mem[ZP + 1] << 8)
    // Effective Address = Base Address + Y
    cpu.Y = 0x04;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_INDY);
    mem.Write(0x4001, 0x02);  // ZP
    mem.Write(0x0002, 0x00);  // Low Byte
    mem.Write(0x0003, 0x80);  // High Byte -> Base: 0x8000
    mem.Write(0x8004, 0x37);  // 0x8000 + 0x04 = 0x8004
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}