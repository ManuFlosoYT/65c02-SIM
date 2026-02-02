#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class INC_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INC_AbsoluteX_Test, INC_AbsoluteX) {
    // Addr = 0x4480 + 0x01 = 0x4481
    // Mem[0x4481] = 0x05 -> 0x06
    cpu.X = 0x01;

    mem[0xFFFC] = INS_INC_ABSX;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4481] = 0x05;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4481], 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
