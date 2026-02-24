#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BEQ_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

// BEQ: Branch if Z = 1

TEST_F(BEQ_Test, BEQ_NoBranch_ZeroClear) {
    cpu.Z = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BEQ_Test, BEQ_Branch_ZeroSet) {
    cpu.Z = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}