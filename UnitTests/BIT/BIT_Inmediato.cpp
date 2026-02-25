#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BIT_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(BIT_Immediate_Test, BIT_Immediate_SetsZeroFlag) {
    // A = 0x0F, Val = 0xF0 -> A & Val = 0x00 -> Z=1
    cpu.A = 0x0F;
    cpu.Z = 0;
    cpu.N = 0;  // Should remain 0
    cpu.V = 0;  // Should remain 0

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BIT_IM);
    bus.Write(0x4001, 0xF0);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x0F);  // A Not modified
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}

TEST_F(BIT_Immediate_Test, BIT_Immediate_ClearsZeroFlag) {
    // A = 0xFF, Val = 0x0F -> A & Val = 0x0F != 0 -> Z=0
    cpu.A = 0xFF;
    cpu.Z = 1;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BIT_IM);
    bus.Write(0x4001, 0x0F);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
}

TEST_F(BIT_Immediate_Test, BIT_Immediate_DoesNotAffectNV) {
    // Immediate mode BIT does not copy bits 7 and 6 to N and V.
    // Operand = 0xC0 (Bits 7 and 6 set)
    cpu.A = 0xFF;
    cpu.N = 0;
    cpu.V = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BIT_IM);
    bus.Write(0x4001, 0xC0);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_FALSE(cpu.N);
    EXPECT_FALSE(cpu.V);
}