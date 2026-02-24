#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BVS_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

// BVS: Branch if V = 1

TEST_F(BVS_Test, BVS_NoBranch_OverflowClear) {
    cpu.V = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BVS);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BVS_Test, BVS_Branch_OverflowSet) {
    cpu.V = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BVS);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}