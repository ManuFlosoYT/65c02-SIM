#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class CPY_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(CPY_Immediate_Test, CPY_Immediate) {
    // Y = 5, M = 5. Result = 0. Z=1, C=1.
    cpu.Y = 0x05;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CPY_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x05);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}

TEST_F(CPY_Immediate_Test, CPY_Immediate_Less) {
    // Y = 4, M = 5. Result = 0xFF. Z=0, C=0. N=1.
    cpu.Y = 0x04;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CPY_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}

TEST_F(CPY_Immediate_Test, CPY_Immediate_Greater) {
    // Y = 6, M = 5. Result = 1. Z=0, C=1. N=0.
    cpu.Y = 0x06;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CPY_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.N);
}