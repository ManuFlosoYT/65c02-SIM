#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BNE_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BNE: Branch if Z = 0

TEST_F(BNE_Test, BNE_NoBranch_ZeroSet) {
    cpu.Z = 1;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BNE;
    mem[0x1001] = 0x05;
    mem[0x1002] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BNE_Test, BNE_Branch_ZeroClear) {
    cpu.Z = 0;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BNE;
    mem[0x1001] = 0x05;
    mem[0x1007] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}
