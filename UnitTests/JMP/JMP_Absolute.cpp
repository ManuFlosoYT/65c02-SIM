#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

class JMP_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(JMP_Absolute_Test, JMP_Absolute) {
    // 0xFFFC: JMP 0x8000
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0xFF (Stop)

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_JMP_ABS);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);
    mem.Write(0x8000, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x8001);
}