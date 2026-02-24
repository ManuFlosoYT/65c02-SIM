#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class CPX_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(CPX_Absolute_Test, CPX_Absolute) {
    // X = 5, Mem = 5
    cpu.X = 0x05;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CPX_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x05);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}