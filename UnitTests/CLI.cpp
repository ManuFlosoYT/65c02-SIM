#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class CLI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLI_Test, CLI_ClearsInterruptDisable) {
    cpu.I = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CLI);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.I);
}