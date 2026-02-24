#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class SBC_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_IndirectX_Test, SBC_IndirectX) {
    cpu.X = 0x04;
    cpu.A = 0x0A;
    cpu.C = 1;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_INDX);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0006, 0x00);
    mem.Write(0x0007, 0x80);
    mem.WriteROM(0x8000, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
}