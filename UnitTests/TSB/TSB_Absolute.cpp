#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TSB_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xFF
    cpu.A = 0xAA;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0x55);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0xFF);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_ClearsZeroFlag) {
    // A = 0x80 (1000 0000)
    // M = 0x80 (1000 0000)
    // A & M != 0 -> Z = 0
    // M = M | A = 0x80
    cpu.A = 0x80;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0x80);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0x80);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsBits) {
    // A = 0x0F (0000 1111)
    // M = 0xF0 (1111 0000)
    // Z = 1
    // M = 0xFF
    cpu.A = 0x0F;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.WriteDirect(0x2000, 0xF0);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x2000), 0xFF);
}