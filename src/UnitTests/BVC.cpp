#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BVC_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// BVC: Branch if V = 0

TEST_F(BVC_Test, BVC_NoBranch_OverflowSet) {
    cpu.V = 1;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BVC);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1002, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BVC_Test, BVC_Branch_OverflowClear) {
    cpu.V = 0;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BVC);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1007, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1008);
}