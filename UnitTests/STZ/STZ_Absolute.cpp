#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STZ_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STZ_Absolute_Test, STZ_Absolute_ExecutesCorrectly) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STZ_ABS);
    bus.Write(0x4001, 0x00);     // Low
    bus.Write(0x4002, 0x20);     // High -> 0x2000
    bus.Write(0x4003, INS_JAM);  // Stop

    bus.Write(0x2000, 0xDD);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x2000), 0x00);
    EXPECT_EQ(cpu.PC, 0x4004);  // 16-bit wrap from FFFF
}