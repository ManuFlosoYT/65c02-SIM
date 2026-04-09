#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDY_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDY_Absolute_Test, LDY_Absolute) {
    // 0xFFFC: LDY 0x8000
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // 0x8000: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x37);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Absolute_Test, LDY_Absolute_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x00);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_Absolute_Test, LDY_Absolute_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x80);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}