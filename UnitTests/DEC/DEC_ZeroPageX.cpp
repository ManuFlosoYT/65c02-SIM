#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class DEC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_ZeroPageX_Test, DEC_ZeroPageX) {
    // Addr = 0x42 + 0x05 = 0x47
    // Mem[0x47] = 0x05 -> 0x04
    cpu.X = 0x05;

    mem[0xFFFC] = INS_DEC_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0047] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0047], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPageX_Test, DEC_ZeroPageX_WrapAround) {
    // Addr = 0x80 + 0xFF = 0x17F -> 0x7F
    // Mem[0x7F] = 0x05 -> 0x04
    cpu.X = 0xFF;

    mem[0xFFFC] = INS_DEC_ZPX;
    mem[0xFFFD] = 0x80;
    mem[0x007F] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x007F], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
