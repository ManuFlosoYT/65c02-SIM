#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STY_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STY_ZeroPage_Test, STY_ZeroPage) {
    cpu.Y = 0x37;

    // 0xFFFC: STY (ZeroPage) 0x42
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STY_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x4002, INS_JAM);  // Stop

    mem.Write(0x0042, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x37);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}