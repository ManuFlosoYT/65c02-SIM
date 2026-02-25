#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ASL_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// ASL: Shift Left One Bit (Memory or Accumulator)
// C <- [7...0] <- 0

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_NoCarry) {
    // 0000 0001 (1) -> 0000 0010 (2). C=0
    cpu.A = 0x01;
    cpu.C = 1;  // Should be cleared

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ASL_A);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x02);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_CarryOut) {
    // 1000 0000 (0x80) -> 0000 0000 (0). C=1
    cpu.A = 0x80;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ASL_A);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ASL_Accumulator_Test, ASL_Accumulator_Negative) {
    // 0100 0000 (0x40) -> 1000 0000 (0x80). N=1
    cpu.A = 0x40;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ASL_A);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}