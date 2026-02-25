#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STZ_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STZ_ZeroPageX_Test, STZ_ZeroPageX_ExecutesCorrectly) {
    cpu.PC = 0xFFFC;
    cpu.X = 0x04;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZPX);
    bus.Write(0x4001, 0x20);  // Base 0x20
    bus.Write(0x4002, INS_JAM);

    // Target: 0x20 + 0x04 = 0x24
    bus.Write(0x0024, 0xBB);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0024), 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}

TEST_F(STZ_ZeroPageX_Test, STZ_ZeroPageX_Wrapping) {
    cpu.PC = 0xFFFC;
    cpu.X = 0xFF;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ZPX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, INS_JAM);

    // Target: 0x80 + 0xFF = 0x17F -> 0x7F (Zero page wrap)
    bus.Write(0x007F, 0xCC);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x007F), 0x00);
    EXPECT_EQ(cpu.PC, 0x4003);
}