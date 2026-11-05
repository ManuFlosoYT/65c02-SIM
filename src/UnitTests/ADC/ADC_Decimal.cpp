#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ADC_Decimal_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ADC_Decimal_Test, ADC_Decimal_Simple) {
    cpu.A = 0x05;
    cpu.D = 1;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(ADC_Decimal_Test, ADC_Decimal_Carry) {
    // BCD decimal wrap: 50 + 51 = 101 -> 0x01 with carry
    cpu.A = 0x50;
    cpu.D = 1;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x51);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x01);
    EXPECT_TRUE(cpu.C);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(ADC_Decimal_Test, ADC_Decimal_CarryIn) {
    cpu.A = 0x09;
    cpu.D = 1;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x01);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x11);
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_Decimal_Test, ADC_Decimal_ZeroAndNegativeFlags) {
    cpu.A = 0x50;
    cpu.D = 1;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IM);
    bus.Write(0x4001, 0x50);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
