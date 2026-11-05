#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class SBC_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};


TEST_F(SBC_Immediate_Test, SBC_Immediate) {
    cpu.A = 0x05;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x03);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x02);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(SBC_Immediate_Test, SBC_Immediate_Borrow) {
    cpu.A = 0x05;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x06);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(SBC_Immediate_Test, SBC_Immediate_BorrowIn) {
    cpu.A = 0x05;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_SBC_IM);
    bus.Write(0x4001, 0x03);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);
}