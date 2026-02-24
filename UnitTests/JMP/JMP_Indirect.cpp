#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class JMP_Indirect_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(JMP_Indirect_Test, JMP_Indirect) {
    // 0xFFFC: JMP (0x8000)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0x00 (Low Byte of Target)
    // 0x8001: 0x90 (High Byte of Target) -> Target 0x9000
    // 0x9000: 0xFF (Stop)

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_JMP_IND);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, 0x80);

    mem.WriteROM(0x8000, 0x00);
    mem.WriteROM(0x8001, 0x90);

    mem.WriteROM(0x9000, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x9001);
}