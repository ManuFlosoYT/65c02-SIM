#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class JMP_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(JMP_AbsoluteX_Test, JMP_AbsoluteX) {
    // 0xFFFC: JMP (0x8000, X)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // X = 4
    // Target = 0x8000 + 4 = 0x8004
    // 0x8004: 0xFF (Stop)

    cpu.X = 0x04;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_JMP_ABSX);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);

    mem.Write(0x8004, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x8005);
}