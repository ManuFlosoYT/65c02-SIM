#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class BVC_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BVC: Branch if V = 0

TEST_F(BVC_Test, BVC_NoBranch_OverflowSet) {
    cpu.V = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BVC);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BVC_Test, BVC_Branch_OverflowClear) {
    cpu.V = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BVC);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}