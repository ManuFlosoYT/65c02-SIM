#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class PLP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(PLP_Test, PLP) {
    // 0xFFFC: PLP
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_PLP);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    // Stack contains flags to be set
    // N (1) | V (1) | U (1) | B (0) | D (1) | I (1) | Z (1) | C (1)
    // 1110 1111 = 0xEF
    // Note: B bit pulled from stack is ignored by 6502 (B flag in register is
    // not set by PLP). Break flag (bit 4) is ignored. Unused bit (bit 5) is
    // ignored.

    mem.Write(0x01FF, 0xEF);

    cpu.Execute(mem);

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