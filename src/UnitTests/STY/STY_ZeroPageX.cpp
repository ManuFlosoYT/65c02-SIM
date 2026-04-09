#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STY_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STY_ZeroPageX_Test, STY_ZeroPageX) {
    cpu.Y = 0x37;
    cpu.X = 0x0F;

    // 0xFFFC: STY (ZeroPageX) 0x80
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STY_ZPX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Target Zero Page Address = 0x80 + 0x0F = 0x8F
    bus.Write(0x008F, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x008F), 0x37);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}