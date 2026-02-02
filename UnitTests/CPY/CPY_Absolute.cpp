#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CPY_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPY_Absolute_Test, CPY_Absolute) {
    // Y = 5, Mem = 5
    cpu.Y = 0x05;

    mem[0xFFFC] = INS_CPY_ABS;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4480] = 0x05;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
