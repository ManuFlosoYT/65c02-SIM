#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class TSX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TSX_Test, TSX) {
    // 0xFFFC: TSX
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSX);
    mem.Write(0x4001, INS_JAM);  // Stop

    // Set SP to non-zero, non-negative value
    cpu.SP = 0x0150;
    cpu.X = 0x00;
    cpu.Z = 1;
    cpu.N = 1;

    // Note: TSX transfers the LOW 8 BITS of SP to X?
    // 6502 SP is 8-bit (offset from 0x0100).
    // In this emu, SP is Word (16-bit).
    // "Transfer Stack Pointer to X". X is 8-bit.
    // So it should transfer the low byte of SP (0x50).

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x50);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TSX_Test, TSX_ZeroFlag) {
    // 0xFFFC: TSX
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x0100;  // Low byte is 0x00
    cpu.X = 0xFF;
    cpu.Z = 0;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TSX_Test, TSX_NegativeFlag) {
    // 0xFFFC: TSX
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_TSX);
    mem.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x0180;  // Low byte is 0x80 (Negative)
    cpu.X = 0x00;
    cpu.N = 0;

    cpu.Execute(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}