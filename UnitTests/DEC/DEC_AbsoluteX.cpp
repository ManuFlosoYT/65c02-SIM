#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class DEC_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_AbsoluteX_Test, DEC_AbsoluteX) {
    // Addr = 0x4480 + 0x01 = 0x4481
    // Mem[0x4481] = 0x05 -> 0x04
    cpu.X = 0x01;

    mem[0xFFFC] = INS_DEC_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x05;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4481], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
