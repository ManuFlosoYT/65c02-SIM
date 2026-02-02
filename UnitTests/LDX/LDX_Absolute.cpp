#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDX_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_Absolute_Test, LDX_Absolute) {
    // 0xFFFC: LDX 0x8000
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0x37
    mem[0xFFFC] = INS_LDX_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x37;

    // We assume Instruction takes 3 bytes (1 opcode + 2 address)
    // Next instruction at 0xFFFF (but we are at FFFC+3 = FFFF)
    // The previous tests used 0xFFFE as the stop opcode, but if the instruction
    // is 3 bytes long: FFFC (Op), FFFD (AL), FFFE (AH) Next Opcode at FFFF.
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x0000);  // FFFF + 1 = 0 (wrap) or just Stop.
    // Wait, let's check LDA_ZeroPage again. It was at FFFC, FFFD (arg), FFFE
    // (stop). ZeroPage is 2 bytes. Absolute is 3 bytes. So FFFC, FFFD, FFFE.
    // Next PC is FFFF. So mem[0xFFFF] = 0xFF is correct. And expected PC is
    // 0x0000 (after reading 0xFF at FFFF and returning?) The loop in
    // CPU::Execute typically runs until Break or max cycles. If 0xFF returns,
    // the PC might be incremented after fetch. Let's check LDA_ZeroPage
    // expectation: EXPECT_EQ(cpu.PC, 0xFFFF); Instruction at FFFC (1 byte) +
    // Arg (1 byte) = 2 bytes. Next is FFFE. Opcode at FFFE is read. PC becomes
    // FFFF. So for Absolute: Instruction at FFFC (1 byte) + Arg (2 bytes) = 3
    // bytes. PC starts FFFC -> FFFD -> FFFE -> FFFF. Next fetches Opcode at
    // FFFF. PC becomes 0x0000.

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Absolute_Test, LDX_Absolute_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;

    mem[0xFFFC] = INS_LDX_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x00;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Absolute_Test, LDX_Absolute_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;

    mem[0xFFFC] = INS_LDX_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x80;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
