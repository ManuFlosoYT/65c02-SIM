#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ROL_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// ROL: Rotate Left through Carry
// [7...1] <- [6...0]
// [0] <- Old Carry
// C <- Old Bit 7

TEST_F(ROL_Accumulator_Test, ROL_Accumulator_Rotate) {
    // 1000 0000 (0x80) -> 0000 0000 (0x00). Old Carry=0 inserted at bit 0.
    // Carry Out = Old Bit 7 (1).
    cpu.A = 0x80;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ROL_A);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ROL_Accumulator_Test, ROL_Accumulator_NoCarryOut) {
    // 0100 0000 (0x40) -> 1000 0000 (0x80) | Old Carry (1) = 1000 0001 (0x81).
    // Carry Out = Old Bit 7 (0).
    cpu.A = 0x40;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ROL_A);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x81);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}