#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/ListaInstrucciones.h"

class RTS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(RTS_Test, RTS_Implied) {
    // 0xFFFC: RTS
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RTS);
    mem.Write(0x4001, INS_JAM);  // Stop

    // Simulate Return Address on Stack
    // Want to return to 0x2035
    // JSR pushes PC+2 (instruction before JSR is 3 bytes usually, or JSR itself
    // is 3 bytes) If JSR is at 0x2032, JSR 0x????, PC becomes 0x2035 after JSR
    // reads address. JSR pushes PCH (0x20), then PCL (0x34) (Address of last
    // byte of JSR instruction?) 6502 JSR pushes address - 1 of target? No. JSR
    // pushes (PC + 2). Wait. JSR is 3 bytes. Op, Lo, Hi. PC points to Op. PC+1
    // Lo, PC+2 Hi. It pushes the address of the third byte (High byte) of the
    // JSR instruction? "The address pushed to the stack is the address of the
    // last byte of the JSR instruction" (PC + 2) RTS pops it and adds 1. So it
    // returns to execution at PC + 2 + 1 = PC + 3 (Instruction after JSR).

    // Let's say we want to return to 0x8000.
    // Stack should contain 0x7FFF.
    // Pushed High then Low?
    // SP starts at 0x01FF.
    // Push High (0x7F) -> 0x01FF
    // Push Low  (0xFF) -> 0x01FE
    // SP is 0x01FD.

    cpu.SP = 0x01FD;
    mem.Write(0x01FE, 0xFF);
    mem.Write(0x01FF, 0x7F);
    mem.Write(0x8000, INS_JAM);  // Stop opcode at return address

    cpu.Execute(mem);

    // Expected PC:
    // RTS sets PC = 0x8000.
    // Loop fetches opcode at 0x8000 (0xFF), increments PC to 0x8001.
    // Executes 0xFF (Stop), returns.
    EXPECT_EQ(cpu.PC, 0x8001);
    EXPECT_EQ(cpu.SP, 0x01FF);  // Stack pointer restored to 0x01FF
}