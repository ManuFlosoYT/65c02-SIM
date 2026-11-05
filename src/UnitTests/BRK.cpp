#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BRK_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(BRK_Test, BRK_Operations) {

    cpu.PC = 0x1000;
    cpu.SP = 0x01FF;

    bus.WriteDirect(0xFFFE, 0x00);
    bus.WriteDirect(0xFFFF, 0x20);
    bus.Write(0x2000, INS_STP);

    bus.Write(0x1000, INS_BRK);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);


    EXPECT_EQ(cpu.PC, 0x2001);

    EXPECT_EQ(bus.ReadDirect(0x0100 + 0xFF), 0x10);
    EXPECT_EQ(bus.ReadDirect(0x0100 + 0xFE), 0x02);

    EXPECT_TRUE(cpu.B);
}