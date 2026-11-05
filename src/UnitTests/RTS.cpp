#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class RTS_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(RTS_Test, RTS_Implied) {
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_RTS);
    bus.Write(0x4001, INS_STP);



    cpu.SP = 0x01FD;
    bus.Write(0x01FE, 0xFF);
    bus.Write(0x01FF, 0x7F);
    bus.WriteDirect(0x8000, INS_STP);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x8001);
    EXPECT_EQ(cpu.SP, 0x01FF);
}