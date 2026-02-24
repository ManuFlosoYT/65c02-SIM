#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class SBC_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_Absolute_Test, SBC_Absolute) {
    // Addr = 0x4480
    // 10 - 5 = 5
    cpu.A = 0x0A;
    cpu.C = 1;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x05);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
}