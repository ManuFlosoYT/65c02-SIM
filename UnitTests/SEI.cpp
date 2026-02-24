#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class SEI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(SEI_Test, SEI_SetsInterruptDisable) {
    cpu.I = 0;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SEI);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.I);
}