#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STZ_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }
    Mem mem;
    CPU cpu;
};

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_ExecutesCorrectly) {
    cpu.PC = 0x1000;
    cpu.isInit = true;
    cpu.X = 0x05;
    mem.Write(0x1000, INS_STZ_ABSX);
    mem.Write(0x1001, 0x00);
    mem.Write(0x1002, 0x20);     // 0x2000
    mem.Write(0x1003, INS_JAM);  // Stop

    // Target: 0x2000 + 0x05 = 0x2005
    mem.Write(0x2005, 0xEE);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(mem[0x2005], 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_PageCrossing) {
    cpu.PC = 0x1000;
    cpu.isInit = true;
    cpu.X = 0xFF;
    mem.Write(0x1000, INS_STZ_ABSX);
    mem.Write(0x1001, 0x00);
    mem.Write(0x1002, 0x20);  // 0x2000
    mem.Write(0x1003, INS_JAM);

    // Target: 0x2000 + 0xFF = 0x20FF
    mem.Write(0x20FF, 0xEE);

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);

    EXPECT_EQ(mem[0x20FF], 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}