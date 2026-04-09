#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TSB_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xAA | 0x55 = 0xFF
    cpu.A = 0xAA;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x55);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0xFF);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_ClearsZeroFlag) {
    // A = 0x01 (0000 0001)
    // M = 0x01 (0000 0001)
    // A & M = 1 != 0 -> Z = 0
    // M = A | M = 0x01 | 0x01 = 0x01
    cpu.A = 0x01;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x01);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0x01);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_SetsBits) {
    // A = 0xF0 (1111 0000)
    // M = 0x0F (0000 1111)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xF0 | 0x0F = 0xFF
    cpu.A = 0xF0;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x0F);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0xFF);
}