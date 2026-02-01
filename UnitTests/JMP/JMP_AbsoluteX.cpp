#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class JMP_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(JMP_AbsoluteX_Test, JMP_AbsoluteX) {
    // 0xFFFC: JMP (0x8000, X)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // X = 4
    // Target = 0x8000 + 4 = 0x8004
    // 0x8004: 0xFF (Stop)

    cpu.X = 0x04;

    mem[0xFFFC] = INS_JMP_ABSX;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;

    mem[0x8004] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x8005);
}
