#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDA_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDA_IndirectX_Test, LDA_IndirectX) {
    cpu.X = 0x04;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_INDX);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0006, 0x00);
    bus.Write(0x0007, 0x80);
    bus.WriteDirect(0x8000, 0x37);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_ZeroFlag) {
    cpu.X = 0x04;
    cpu.A = 0xFF;
    cpu.Z = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_INDX);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0006, 0x00);
    bus.Write(0x0007, 0x80);
    bus.WriteDirect(0x8000, 0x00);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_NegativeFlag) {
    cpu.X = 0x04;
    cpu.N = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_INDX);
    bus.Write(0x4001, 0x02);
    bus.Write(0x0006, 0x00);
    bus.Write(0x0007, 0x80);
    bus.WriteDirect(0x8000, 0x88);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x88);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_Wrapping) {
    cpu.X = 0x02;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_INDX);
    bus.Write(0x4001, 0xFF);

    bus.Write(0x0001, 0x34);
    bus.Write(0x0002, 0x12);

    bus.Write(0x1234, 0x99);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x99);
}