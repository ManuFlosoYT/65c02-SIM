#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class TXS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TXS_Test, TXS) {
    // 0xFFFC: TXS
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TXS);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x80;
    cpu.SP = 0x01FF;  // Default

    // TXS copies X to SP (Low byte). High byte fixed at 0x01?
    // "Transfer X to Stack Pointer".
    // 6502 behavior: SP = X. High byte is implied 0x01.
    // In this emu, SP is Word.
    // If the implementation follows standard 6502, SP should become 0x0180.

    cpu.Execute(mem);

    EXPECT_EQ(cpu.SP, 0x0180);
    // Flags not affected
}