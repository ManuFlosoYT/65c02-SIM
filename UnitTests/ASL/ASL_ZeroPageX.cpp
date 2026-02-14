#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class ASL_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ASL_ZeroPageX_Test, ASL_ZeroPageX) {
    // Addr = 0x42 + 0x05 = 0x47
    // Mem[0x47] = 0x01 -> 0x02
    cpu.X = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ASL_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x01);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0047], 0x02);
}