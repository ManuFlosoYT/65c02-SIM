#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDY_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX) {
    cpu.X = 0x01;
    // 0xFFFC: LDY 0x8000, X
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0x01 = 0x8001
    // 0x8001: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8001, 0x37);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_PageCrossing) {
    cpu.X = 0xFF;
    // 0xFFFC: LDY 0x8000, X
    // 0xFFFD: 0x00
    // 0xFFFE: 0x80
    // Address: 0x8000 + 0xFF = 0x80FF
    // 0x80FF: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x80FF, 0x37);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;
    cpu.X = 0x01;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8001, 0x00);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_AbsoluteX_Test, LDY_AbsoluteX_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;
    cpu.X = 0x01;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDY_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8001, 0x80);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}