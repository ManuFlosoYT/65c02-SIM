#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STA_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STA_ZeroPageX_Test, STA_ZeroPageX) {
    cpu.A = 0x37;
    cpu.X = 0x0F;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_ZPX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, INS_STP);

    bus.Write(0x008F, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(bus.ReadDirect(0x008F), 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(STA_ZeroPageX_Test, STA_ZeroPageX_WrapAround) {
    cpu.A = 0x22;
    cpu.X = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_ZPX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, INS_STP);

    bus.Write(0x007F, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(bus.ReadDirect(0x007F), 0x22);
}