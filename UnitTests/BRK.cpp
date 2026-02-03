#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BRK_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(BRK_Test, BRK_Operations) {
    // BRK: Force Interrupt
    // 1. PC + 2 pushed to stack (High byte first)
    // 2. Status Register (P) pushed to stack with Break flag (B) set
    // 3. Interrupt Disable (I) set
    // 4. PC loaded from Interrupt Vector (0xFFFE/0xFFFF)

    cpu.PC = 0x1000;
    cpu.SP = 0x01FF;

    // Interrupt Vector
    mem.Write(0xFFFE, 0x00);
    mem.Write(0xFFFF, 0x20);  // 0x2000
    mem.Write(0x2000, INS_JAM);  // STOP

    mem.Write(0x1000, INS_BRK);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Ejecutar(mem);

    // PC pushed is Address of BRK + 2 = 1002 (Standard 6502)
    // Stack:
    // FF: High PC (0x10)
    // FE: Low PC (0x02)
    // FD: Status (B bit set, I bit not affected)

    EXPECT_EQ(cpu.PC, 0x2001);  // 2000 + 1 (read 0xFF)

    EXPECT_EQ(mem[0x0100 + 0xFF], 0x10);
    EXPECT_EQ(mem[0x0100 + 0xFE], 0x02);

    EXPECT_TRUE(cpu.B);  // Break Flag Set
}