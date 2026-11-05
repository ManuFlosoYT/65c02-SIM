#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ADC_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ADC_Immediate_Test, ADC_Immediate) {
    cpu.A = 0x00;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}

TEST_F(ADC_Immediate_Test, ADC_Immediate_CarryIn) {
    cpu.A = 0x00;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x06);
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_Immediate_Test, ADC_Immediate_CarryOut) {
    cpu.A = 0xFF;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x01);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
}

TEST_F(ADC_Immediate_Test, ADC_Immediate_Overflow) {
    // Signed overflow: 127 + 1 = -128 sets V flag
    cpu.A = 0x7F;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x01);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_TRUE(cpu.V);
    EXPECT_TRUE(cpu.N);
}