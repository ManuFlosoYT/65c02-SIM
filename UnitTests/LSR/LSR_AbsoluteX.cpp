#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LSR_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(LSR_AbsoluteX_Test, LSR_AbsoluteX) {
    // Addr = 0x4480 + 1 = 0x4481
    cpu.X = 0x01;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LSR_ABSX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4481, 0x02);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x4481], 0x01);
}