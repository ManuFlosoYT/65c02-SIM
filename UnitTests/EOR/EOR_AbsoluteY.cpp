#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class EOR_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_AbsoluteY_Test, EOR_AbsoluteY) {
    cpu.Y = 0x01;
    cpu.A = 0xFF;
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_ABSY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);  // 0x4480
    mem.Write(0x4481, 0x0F);  // 0x4480 + 0x01. 0xFF ^ 0x0F = 0xF0
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}