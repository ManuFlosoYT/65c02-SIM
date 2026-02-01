#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STY_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STY_ZeroPage_Test, STY_ZeroPage) {
    cpu.Y = 0x37;

    // 0xFFFC: STY (ZeroPage) 0x42
    mem[0xFFFC] = INS_STY_ZP;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0xFF;  // Stop

    mem[0x0042] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x0042], 0x37);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
