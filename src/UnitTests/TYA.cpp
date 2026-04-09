#include <gtest/gtest.h>

#include "Hardware/Core/Bus.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TYA_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(TYA_Test, TYA) {
    // 0xFFFC: TYA
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TYA);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x42;
    cpu.A = 0x00;

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_ZeroFlag) {
    // 0xFFFC: TYA
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TYA);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x00;
    cpu.A = 0x42;

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(TYA_Test, TYA_NegativeFlag) {
    // 0xFFFC: TYA
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TYA);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.Y = 0x80;
    cpu.A = 0x00;

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}