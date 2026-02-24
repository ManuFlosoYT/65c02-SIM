#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class CLD_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLD_Test, CLD_ClearsDecimal) {
    cpu.D = 1;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CLD);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_FALSE(cpu.D);
}