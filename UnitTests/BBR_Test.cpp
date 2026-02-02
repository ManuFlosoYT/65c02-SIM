#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BBR_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BBRx: Branch if bit x of Accumulator is Reset (0).
// 2-byte instruction: Opcode, Offset.
// Address Mode: Relative

TEST_F(BBR_Test, BBR0_BranchTaken) {
    cpu.PC = 0xFFFC;
    mem[0xFFFC] = INS_BBR0;
    mem[0xFFFD] = 0x05;  // Offset +5
    mem[0xFFFE] = 0xFF;  // Stop if not taken (or part of skip)

    // Accumulator Value: Bit 0 is 0
    cpu.A = 0xFE;  // 1111 1110

    // Target: PC (after fetch offset at 0xFFFD) = 0xFFFE.
    // 0xFFFE + 5 = 0x0003 (Wraps)
    // We put Stop at 0x0003
    mem[0x0003] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x0004);  // 0x0003 + 1 (Stop fetch) = 0x0004
}

TEST_F(BBR_Test, BBR0_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    mem[0xFFFC] = INS_BBR0;
    mem[0xFFFD] = 0x05;  // Offset
    mem[0xFFFE] = 0xFF;  // Stop if not taken

    // Accumulator Value: Bit 0 is 1
    cpu.A = 0x01;  // 0000 0001 (Bit 0 set)

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFF);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}

TEST_F(BBR_Test, BBR7_BranchTaken) {
    cpu.PC = 0xFFFC;
    mem[0xFFFC] = INS_BBR7;
    mem[0xFFFD] = 0x04;  // Offset +4
    mem[0xFFFE] = 0xFF;  // Stop if not taken

    // Accumulator Value: Bit 7 is 0
    cpu.A = 0x7F;  // 0111 1111

    // Target: PC (0xFFFE) + 4 = 0x0002
    mem[0x0002] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x0003);  // 0x0002 + 1 (Stop fetch) = 0x0003
}

TEST_F(BBR_Test, BBR7_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    mem[0xFFFC] = INS_BBR7;
    mem[0xFFFD] = 0x04;
    mem[0xFFFE] = 0xFF;  // Stop

    // Accumulator Value: Bit 7 is 1
    cpu.A = 0x80;  // 1000 0000

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFF);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}
