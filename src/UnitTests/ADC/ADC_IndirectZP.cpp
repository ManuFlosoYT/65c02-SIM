#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ADC_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Basic) {
    cpu.A = 0x10;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IND_ZP);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0005, 0x00);
    bus.Write(0x0006, 0x80);     // Pointer -> 0x8000
    bus.WriteDirect(0x8000, 0x10);  // Value

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x20);  // 0x10 + 0x10 + 0 = 0x20
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Carry) {
    cpu.A = 0x10;
    cpu.C = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IND_ZP);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0005, 0x00);
    bus.Write(0x0006, 0x80);     // Pointer -> 0x8000
    bus.WriteDirect(0x8000, 0x10);  // Value

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x21);  // 0x10 + 0x10 + 1 = 0x21
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_Overflow) {
    cpu.A = 0x50;  // Positive (+80)
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IND_ZP);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0010, 0x00);
    bus.Write(0x0011, 0x90);     // Pointer -> 0x9000
    bus.WriteDirect(0x9000, 0x50);  // Positive (+80)

    // 0x50 + 0x50 = 0xA0 (-96) -> Overflow
    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xA0);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);  // Overflow set
    EXPECT_FALSE(cpu.C);
}

TEST_F(ADC_IndirectZP_Test, ADC_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x01;
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ADC_IND_ZP);
    bus.Write(0x4001, 0xFF);     // ZP Boundary
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x00FF, 0x10);     // Low
    bus.Write(0x0000, 0xC0);     // High (wrapped) -> 0xC010
    bus.WriteDirect(0xC010, 0xFE);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xFF);  // 0x01 + 0xFE = 0xFF
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.N);
}