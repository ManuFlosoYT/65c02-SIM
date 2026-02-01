#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BCS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BCS: Branch if C = 1

TEST_F(BCS_Test, BCS_NoBranch_CarryClear) {
    cpu.C = 0;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BCS;
    mem[0x1001] = 0x05;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1002);
}

TEST_F(BCS_Test, BCS_Branch_CarrySet) {
    cpu.C = 1;
    cpu.PC = 0x1000;

    mem[0x1000] = INS_BCS;
    mem[0x1001] = 0x05;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1007);
}
