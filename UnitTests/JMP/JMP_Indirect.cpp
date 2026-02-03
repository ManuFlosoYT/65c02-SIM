#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class JMP_Indirect_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(JMP_Indirect_Test, JMP_Indirect) {
    // 0xFFFC: JMP (0x8000)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0x00 (Low Byte of Target)
    // 0x8001: 0x90 (High Byte of Target) -> Target 0x9000
    // 0x9000: 0xFF (Stop)

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_JMP_IND;
    mem[0x4001] = 0x00;
    mem[0x4002] = 0x80;

    mem[0x8000] = 0x00;
    mem[0x8001] = 0x90;

    mem[0x9000] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x9001);
}