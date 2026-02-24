#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class RMB_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(RMB_Test, RMB0_ClearsBit0) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB0);
    mem.Write(0x4001, 0x10);     // Zero Page Address
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0010, 0xFF);     // Initial value: 1111 1111

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xFE);  // Expected: 1111 1110 (Bit 0 cleared)
}

TEST_F(RMB_Test, RMB1_ClearsBit1) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB1);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);  // 1111 1111

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xFD);  // 1111 1101 (Bit 1 cleared)
}

TEST_F(RMB_Test, RMB2_ClearsBit2) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB2);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xFB);  // 1111 1011
}

TEST_F(RMB_Test, RMB3_ClearsBit3) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB3);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xF7);  // 1111 0111
}

TEST_F(RMB_Test, RMB4_ClearsBit4) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB4);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xEF);  // 1110 1111
}

TEST_F(RMB_Test, RMB5_ClearsBit5) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB5);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xDF);  // 1101 1111
}

TEST_F(RMB_Test, RMB6_ClearsBit6) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB6);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0xBF);  // 1011 1111
}

TEST_F(RMB_Test, RMB7_ClearsBit7) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_RMB7);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0xFF);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x7F);  // 0111 1111
}