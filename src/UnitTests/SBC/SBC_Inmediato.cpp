#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class SBC_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// SBC: A = A - M - (1 - C)
// Note: 6502 subtraction requires C=1 for "no borrow".

TEST_F(SBC_Immediate_Test, SBC_Immediate) {
    // 5 - 3 = 2. C=1 (no borrow)
    // A=5, M=3, C=1. Result = 5 - 3 - 0 = 2.
    cpu.A = 0x05;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x03);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x02);
    EXPECT_TRUE(cpu.C);  // No borrow occurred, so C remains 1
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(SBC_Immediate_Test, SBC_Immediate_Borrow) {
    // 5 - 6 = -1 (0xFF). C=1 (no borrow start).
    // A=5, M=6, C=1. Result = 5 - 6 - 0 = -1 (0xFF).
    cpu.A = 0x05;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x06);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.C);  // Borrow occurred, C=0
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(SBC_Immediate_Test, SBC_Immediate_BorrowIn) {
    // 5 - 3 - 1 = 1. C=0 (borrow incoming)
    // A=5, M=3, C=0. Result = 5 - 3 - 1 = 1.
    cpu.A = 0x05;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x03);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);  // No borrow occurred for this operation, C=1
}