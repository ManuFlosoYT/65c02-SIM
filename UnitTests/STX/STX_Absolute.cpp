#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STX_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STX_Absolute_Test, STX_Absolute) {
    cpu.X = 0x37;

    // 0xFFFC: STX (Absolute) 0x8000
    mem[0xFFFC] = INS_STX_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0xFFFF] = 0xFF;  // Stop

    mem[0x8000] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x8000], 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
