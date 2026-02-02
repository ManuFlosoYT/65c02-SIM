#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BEQ_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BEQ: Branch if Z = 1

TEST_F(BEQ_Test, BEQ_NoBranch_ZeroClear) {
    cpu.Z = 0;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BEQ;
    mem[0x1001] = 0x05;
    mem[0x1002] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BEQ_Test, BEQ_Branch_ZeroSet) {
    cpu.Z = 1;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BEQ;
    mem[0x1001] = 0x05;
    mem[0x1007] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}
