#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STA_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_ZeroPageX_Test, STA_ZeroPageX) {
    cpu.A = 0x37;
    cpu.X = 0x0F;

    // 0xFFFC: STA (ZeroPageX) 0x80
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, INS_JAM);  // Stop

    // Target Zero Page Address = 0x80 + 0x0F = 0x8F
    mem.Write(0x008F, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(mem[0x008F], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(STA_ZeroPageX_Test, STA_ZeroPageX_WrapAround) {
    cpu.A = 0x22;
    cpu.X = 0xFF;

    // 0xFFFC: STA (ZeroPageX) 0x80
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, INS_JAM);  // Stop

    // Target Address = (0x80 + 0xFF) & 0xFF = 0x7F
    mem.Write(0x007F, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(mem[0x007F], 0x22);
}