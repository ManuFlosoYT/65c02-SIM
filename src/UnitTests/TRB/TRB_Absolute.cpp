#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TRB_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(TRB_Absolute_Test, TRB_Absolute_SetsZeroFlag) {
    cpu.A = 0xAA;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TRB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0x55);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0x55);
}

TEST_F(TRB_Absolute_Test, TRB_Absolute_ClearsZeroFlag_And_ResetsBits) {
    cpu.A = 0xFF;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TRB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0xFF);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0x00);
}

TEST_F(TRB_Absolute_Test, TRB_Absolute_PartialReset) {
    cpu.A = 0xF0;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TRB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0xAA);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0x0A);
}