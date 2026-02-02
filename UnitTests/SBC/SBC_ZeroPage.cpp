#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class SBC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_ZeroPage_Test, SBC_ZeroPage) {
    // 10 - 5 = 5. C=1.
    cpu.A = 0x0A;
    cpu.C = 1;

    mem[0xFFFC] = INS_SBC_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x05;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.C);
}
