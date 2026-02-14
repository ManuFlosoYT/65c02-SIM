#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class PHX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PHX_Test, PHX) {
    // 0xFFFC: PHX
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_PHX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x42;
    cpu.SP = 0x01FF;

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x01FF], 0x42);
    EXPECT_EQ(cpu.SP, 0x01FE);
}