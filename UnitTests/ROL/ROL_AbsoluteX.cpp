#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ROL_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ROL_AbsoluteX_Test, ROL_AbsoluteX) {
    // Mem = 0x80 -> 0x00 (Shift Left, C=0 into bit 0).
    // Carry Out = Old Bit 7 (1).
    cpu.C = 0;
    cpu.X = 0x04;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ROL_ABSX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x20);  // 0x2080 + X(4) = 0x2084
    mem.Write(0x2084, 0x80);
    mem.Write(0x4003, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2084], 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
}