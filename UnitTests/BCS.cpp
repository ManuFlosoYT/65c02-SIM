#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BCS_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// BCS: Branch if C = 1

TEST_F(BCS_Test, BCS_NoBranch_CarryClear) {
    cpu.C = 0;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BCS);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1002, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BCS_Test, BCS_Branch_CarrySet) {
    cpu.C = 1;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BCS);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1007, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1008);
}