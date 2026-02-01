#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_ZeroPageX_Test, SBC_ZeroPageX) {
    // Addr = 0x42 + 0x01 = 0x43
    // 10 - 5 = 5.
    cpu.A = 0x0A;
    cpu.X = 0x01;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0043] = 0x05;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}
