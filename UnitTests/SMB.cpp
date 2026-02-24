#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class SMB_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

// SMB should SET the bit to 1. 0 -> 1, 1 -> 1.

TEST_F(SMB_Test, SMB0_SetsBit0) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB0);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0010, 0x00);     // 0000 0000

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x01);  // 0000 0001
}

TEST_F(SMB_Test, SMB0_AlreadySet) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB0);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0010, 0x01);     // 0000 0001

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010],
              0x01);  // Should stay 0000 0001. If XOR, it would become 0x00.
}

TEST_F(SMB_Test, SMB1_SetsBit1) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB1);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x02);  // 0000 0010
}

TEST_F(SMB_Test, SMB2_SetsBit2) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB2);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x04);  // 0000 0100
}

TEST_F(SMB_Test, SMB3_SetsBit3) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB3);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x08);  // 0000 1000
}

TEST_F(SMB_Test, SMB4_SetsBit4) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB4);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x10);  // 0001 0000
}

TEST_F(SMB_Test, SMB5_SetsBit5) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB5);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x20);  // 0010 0000
}

TEST_F(SMB_Test, SMB6_SetsBit6) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB6);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x40);  // 0100 0000
}

TEST_F(SMB_Test, SMB7_SetsBit7) {
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SMB7);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);
    mem.Write(0x0010, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0010], 0x80);  // 1000 0000
}