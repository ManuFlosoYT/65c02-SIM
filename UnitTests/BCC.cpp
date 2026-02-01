#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BCC_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BCC: Branch if C = 0

TEST_F(BCC_Test, BCC_NoBranch_CarrySet) {
    // C=1. Should not branch.
    cpu.C = 1;
    cpu.PC = 0xFFFC;

    // BCC +0x5 (0x05)
    mem[0xFFFC] = INS_BCC;
    mem[0xFFFD] = 0x05;
    mem[0xFFFE] = 0x00;  // Next instruction

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFE);  // Did not take branch, PC advanced by 2
}

TEST_F(BCC_Test, BCC_Branch_CarryClear) {
    // C=0. Should branch.
    cpu.C = 0;
    cpu.PC = 0xFFFC;

    // BCC +0x5
    // Target = PC + 2 + 5 = 0xFFFE + 5 = 0x0003 (Wrapping/Overflow behavior
    // depends on PC width, assuming 16-bit wrap) Actually PC is incremented
    // reading opcode (FFFD), then operand (FFFE). Branch adds to PC. 6502
    // Relative branching is: NewPC = PC + 2 + Offset. Let's keep it simple:
    // Start at 0x1000.

    cpu.PC = 0x1000;

    mem[0x1000] = INS_BCC;
    mem[0x1001] = 0x05;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1007);  // 0x1000 + 2 + 5
}

TEST_F(BCC_Test, BCC_Branch_Backward) {
    // C=0. Branch backward -5 (0xFB as signed 8-bit)
    cpu.C = 0;
    cpu.PC = 0x1010;

    mem[0x1010] = INS_BCC;
    mem[0x1011] = 0xFB;  // -5

    cpu.Ejecutar(mem);

    // 0x1010 + 2 - 5 = 0x1012 - 5 = 0x100D
    EXPECT_EQ(cpu.PC, 0x100D);
}
