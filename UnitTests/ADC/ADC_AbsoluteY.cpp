#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class ADC_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_AbsoluteY_Test, ADC_AbsoluteY) {
    // Addr = 0x4480 + 1 = 0x4481
    // 0 + Mem[0x4481](5) = 5
    cpu.A = 0x00;
    cpu.Y = 0x01;
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_ABSY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4481, 0x05);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
}