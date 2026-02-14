#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class BCS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BCS: Branch if C = 1

TEST_F(BCS_Test, BCS_NoBranch_CarryClear) {
    cpu.C = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BCS);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BCS_Test, BCS_Branch_CarrySet) {
    cpu.C = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BCS);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}