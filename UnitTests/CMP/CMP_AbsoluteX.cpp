#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class CMP_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_AbsoluteX_Test, CMP_AbsoluteX) {
    cpu.A = 0x05;
    cpu.X = 0x01;

    mem[0xFFFC] = INS_CMP_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x05;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}
