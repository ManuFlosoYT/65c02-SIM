#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class AND_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_AbsoluteX_Test, AND_AbsoluteX) {
    cpu.X = 0x01;
    cpu.A = 0xFF;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ABSX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);  // 0x4480
    mem.Write(0x4481, 0x37);  // 0x4480 + 0x01 = 0x4481
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}