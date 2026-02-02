#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STZ_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_Absolute_Test, STZ_Absolute_ExecutesCorrectly) {
    cpu.PC = 0xFFFC;
    mem[0xFFFC] = INS_STZ_ABS;
    mem[0xFFFD] = 0x00;  // Low
    mem[0xFFFE] = 0x20;  // High -> 0x2000
    mem[0xFFFF] = INS_JAM;  // Stop

    mem[0x2000] = 0xDD;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2000], 0x00);
    EXPECT_EQ(cpu.PC, 0x0000);  // 16-bit wrap from FFFF
}
