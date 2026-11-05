#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BCC_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};


TEST_F(BCC_Test, BCC_NoBranch_CarrySet) {
    cpu.C = 1;
    cpu.PC = 0xFFFC;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BCC);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(BCC_Test, BCC_Branch_CarryClear) {
    cpu.C = 0;
    cpu.PC = 0xFFFC;

    // Relative branch address wrap across 0xFFFF boundary

    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BCC);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1007, INS_STP);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1008);
}

TEST_F(BCC_Test, BCC_Branch_Backward) {
    cpu.C = 0;
    cpu.PC = 0x1010;

    bus.Write(0x1010, INS_BCC);
    bus.Write(0x1011, 0xFB);
    bus.Write(0x100D, INS_STP);

    bus.WriteDirect(0xFFFC, 0x10);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x100E);
}