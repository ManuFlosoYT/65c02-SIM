#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STZ_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_ExecutesCorrectly) {

    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_STP);

    bus.Write(0x0042, 0xAA);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(STZ_ZeroPage_Test, STZ_ZeroPage_DoesNotAffectFlags) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_STP);

    bus.Write(0x0042, 0xAA);

    cpu.Z = 0;
    cpu.N = 1;
    cpu.C = 1;
    cpu.V = 1;

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_EQ(cpu.Z, 0);
    EXPECT_EQ(cpu.N, 1);
    EXPECT_EQ(cpu.C, 1);
    EXPECT_EQ(cpu.V, 1);
}