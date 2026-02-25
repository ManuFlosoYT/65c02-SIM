#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class INY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(INY_Test, INY) {
    // Y = 0x05 -> 0x06
    cpu.Y = 0x05;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INY);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INY_Test, INY_ZeroFlag) {
    // Y = 0xFF -> 0x00
    cpu.Y = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INY);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INY_Test, INY_NegativeFlag) {
    // Y = 0x7F -> 0x80
    cpu.Y = 0x7F;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INY);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}