#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

class BPL_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BPL: Branch if N = 0

TEST_F(BPL_Test, BPL_NoBranch_NegativeSet) {
    cpu.N = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BPL);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BPL_Test, BPL_Branch_NegativeClear) {
    cpu.N = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BPL);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}