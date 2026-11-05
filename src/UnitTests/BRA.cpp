#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BRA_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(BRA_Test, BRA_PositiveOffset) {
    cpu.PC = 0x1000;
    bus.Write(0x1000, INS_BRA);
    bus.Write(0x1001, 0x05);
    bus.Write(0x1007, INS_STP);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x1008);
}

TEST_F(BRA_Test, BRA_NegativeOffset) {
    cpu.PC = 0x1010;
    bus.Write(0x1010, INS_BRA);
    bus.Write(0x1011, 0xFB);
    // Destination: 0x1010 + 2 - 5 = 0x1012 - 5 = 0x100D
    bus.Write(0x100D, INS_STP);

    bus.WriteDirect(0xFFFC, 0x10);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x100E);
}

TEST_F(BRA_Test, BRA_MaxPositiveOffset) {
    cpu.PC = 0x2000;
    bus.Write(0x2000, INS_BRA);
    bus.Write(0x2001, 0x7F);
    bus.Write(0x2081, INS_STP);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x20);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x2082);
}

TEST_F(BRA_Test, BRA_MaxNegativeOffset) {
    cpu.PC = 0x3000;
    bus.Write(0x3000, INS_BRA);
    bus.Write(0x3001, 0x80);
    bus.Write(0x2F82, INS_STP);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x30);
    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x2F83);
}