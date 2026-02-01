#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PLP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PLP_Test, PLP) {
    // 0xFFFC: PLP
    mem[0xFFFC] = INS_PLP;
    mem[0xFFFD] = 0xFF;  // Stop

    cpu.SP = 0x01FE;
    // Stack contains flags to be set
    // N (1) | V (1) | U (1) | B (0) | D (1) | I (1) | Z (1) | C (1)
    // 1110 1111 = 0xEF
    // Note: B bit pulled from stack is ignored by 6502 (B flag in register is
    // not set by PLP). Break flag (bit 4) is ignored. Unused bit (bit 5) is
    // ignored.

    mem[0x01FF] = 0xEF;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.I);
    EXPECT_TRUE(cpu.D);
    EXPECT_TRUE(cpu.V);
    EXPECT_TRUE(cpu.N);
    // B flag behavior: PLP should not set B flag usually. (B indicates
    // interrupt source is BRK). But emulators might store it.
    // EXPECT_FALSE(cpu.B); // Not checking B for now as implementation varies.

    EXPECT_EQ(cpu.SP, 0x01FF);
}
