#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LSR_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(LSR_ZeroPageX_Test, LSR_ZeroPageX) {
    // Addr = 0x42 + 2 = 0x44
    // Mem[0x44] = 0x02 -> 0x01
    cpu.X = 0x02;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LSR_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0044, 0x02);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0044], 0x01);
}