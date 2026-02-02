#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class TXS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TXS_Test, TXS) {
    // 0xFFFC: TXS
    mem[0xFFFC] = INS_TXS;
    mem[0xFFFD] = INS_JAM;  // Stop

    cpu.X = 0x80;
    cpu.SP = 0x01FF;  // Default

    // TXS copies X to SP (Low byte). High byte fixed at 0x01?
    // "Transfer X to Stack Pointer".
    // 6502 behavior: SP = X. High byte is implied 0x01.
    // In this emu, SP is Word.
    // If the implementation follows standard 6502, SP should become 0x0180.

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.SP, 0x0180);
    // Flags not affected
}
