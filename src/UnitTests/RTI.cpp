#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class RTI_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(RTI_Test, RTI_Operations) {

    cpu.SP = 0x01FC;
    // Pushed values (e.g. from BRK)
    bus.Write(0x0100 + 0xFD, 0b11000000);
    bus.Write(0x0100 + 0xFE, 0x02);
    bus.Write(0x0100 + 0xFF, 0x10);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RTI);
    bus.Write(0x4001, INS_STP);
    bus.Write(0x1002, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1003);
    EXPECT_TRUE(cpu.N);
    EXPECT_TRUE(cpu.V);
    EXPECT_EQ(cpu.SP, 0x01FF);
}