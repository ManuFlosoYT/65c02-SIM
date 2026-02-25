#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class INX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(INX_Test, INX) {
    // X = 0x05 -> 0x06
    cpu.X = 0x05;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INX);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INX_Test, INX_ZeroFlag) {
    // X = 0xFF -> 0x00
    cpu.X = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INX);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INX_Test, INX_NegativeFlag) {
    // X = 0x7F -> 0x80
    cpu.X = 0x7F;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_INX);
    bus.Write(0x4001, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}