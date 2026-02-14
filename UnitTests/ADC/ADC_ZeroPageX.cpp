#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class ADC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ADC_ZeroPageX_Test, ADC_ZeroPageX) {
    // Addr = 0x42 + 0x01 = 0x43
    // 0 + Mem[0x43](5) = 5
    cpu.A = 0x00;
    cpu.X = 0x01;
    cpu.C = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ADC_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0043, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
}