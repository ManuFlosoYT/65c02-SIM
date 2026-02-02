#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LSR_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LSR_ZeroPageX_Test, LSR_ZeroPageX) {
    // Addr = 0x42 + 2 = 0x44
    // Mem[0x44] = 0x02 -> 0x01
    cpu.X = 0x02;

    mem[0xFFFC] = INS_LSR_ZPX;
    mem[0xFFFD] = 0x42;
    mem[0x0044] = 0x02;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0044], 0x01);
}
