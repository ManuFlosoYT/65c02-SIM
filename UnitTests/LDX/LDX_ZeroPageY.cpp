#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDX_ZeroPageY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY) {
    cpu.Y = 0x04;
    // 0xFFFC: LDX 0x42, Y
    // 0xFFFD: 0x42
    // Address: 0x42 + 0x04 = 0x46
    // 0x0046: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ZPY);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0046, 0x37);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_WrapAround) {
    cpu.Y = 0xFF;
    // 0xFFFC: LDX 0x80, Y
    // 0xFFFD: 0x80
    // Address: (0x80 + 0xFF) & 0xFF = 0x7F
    // 0x007F: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ZPY);
    bus.Write(0x4001, 0x80);
    bus.Write(0x007F, 0x37);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;
    cpu.Y = 0x0A;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ZPY);
    bus.Write(0x4001, 0x42);
    bus.Write(0x004C, 0x00);  // 0x42 + 0x0A
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_ZeroPageY_Test, LDX_ZeroPageY_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;
    cpu.Y = 0x0A;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ZPY);
    bus.Write(0x4001, 0x42);
    bus.Write(0x004C, 0x80);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}