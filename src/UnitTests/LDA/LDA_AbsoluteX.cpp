#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDA_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX) {
    cpu.X = 0x01;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_ABSX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, 0x44);
    bus.Write(0x4481, 0x37);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX_ZeroFlag) {
    cpu.X = 0x01;
    cpu.Z = 0;
    cpu.A = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_ABSX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, 0x44);
    bus.Write(0x4481, 0x00);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX_NegativeFlag) {
    cpu.X = 0x01;
    cpu.N = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_ABSX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, 0x44);
    bus.Write(0x4481, 0x81);
    bus.Write(0x4003, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x81);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}