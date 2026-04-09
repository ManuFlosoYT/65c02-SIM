#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class PLX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(PLX_Test, PLX) {
    // 0xFFFC: PLX
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_PLX);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    bus.Write(0x01FF, 0x42);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_EQ(cpu.SP, 0x01FF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLX_Test, PLX_ZeroFlag) {
    // 0xFFFC: PLX
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_PLX);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    bus.Write(0x01FF, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(PLX_Test, PLX_NegativeFlag) {
    // 0xFFFC: PLX
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_PLX);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.SP = 0x01FE;
    bus.Write(0x01FF, 0x80);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}