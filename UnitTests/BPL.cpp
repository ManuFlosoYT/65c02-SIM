#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BPL_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// BPL: Branch if N = 0

TEST_F(BPL_Test, BPL_NoBranch_NegativeSet) {
    cpu.N = 1;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BPL);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1002, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BPL_Test, BPL_Branch_NegativeClear) {
    cpu.N = 0;
    cpu.PC = 0x1000;

    bus.Write(0x1000, INS_BPL);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1007, INS_JAM);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1008);
}