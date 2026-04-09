#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STZ_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_ExecutesCorrectly) {
    cpu.PC = 0x1000;
    cpu.isInit = true;
    cpu.X = 0x05;
    bus.Write(0x1000, INS_STZ_ABSX);
    bus.Write(0x1001, 0x00);
    bus.Write(0x1002, 0x20);     // 0x2000
    bus.Write(0x1003, INS_JAM);  // Stop

    // Target: 0x2000 + 0x05 = 0x2005
    bus.Write(0x2005, 0xEE);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x2005), 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}

TEST_F(STZ_AbsoluteX_Test, STZ_AbsoluteX_PageCrossing) {
    cpu.PC = 0x1000;
    cpu.isInit = true;
    cpu.X = 0xFF;
    bus.Write(0x1000, INS_STZ_ABSX);
    bus.Write(0x1001, 0x00);
    bus.Write(0x1002, 0x20);  // 0x2000
    bus.Write(0x1003, INS_JAM);

    // Target: 0x2000 + 0xFF = 0x20FF
    bus.Write(0x20FF, 0xEE);

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x10);
    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x20FF), 0x00);
    EXPECT_EQ(cpu.PC, 0x1004);
}