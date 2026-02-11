#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class EOR_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_IndirectY_Test, EOR_IndirectY) {
    cpu.Y = 0x04;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_INDY);
    mem.Write(0x4001, 0x02);  // ZP
    mem.Write(0x0002, 0x00);  // Low Byte
    mem.Write(0x0003, 0x80);  // High Byte -> Base: 0x8000
    mem.Write(0x8004, 0x0F);  // 0x8000 + 0x04. 0xFF ^ 0x0F = 0xF0
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}