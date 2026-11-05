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
    cpu.A = 0xAA;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x55);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0xFF);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_ClearsZeroFlag) {
    cpu.A = 0x01;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x01);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0x01);
}

TEST_F(TSB_ZeroPage_Test, TSB_ZeroPage_SetsBits) {
    cpu.A = 0xF0;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TSB_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x0F);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(bus.ReadDirect(0x0020), 0xFF);
}