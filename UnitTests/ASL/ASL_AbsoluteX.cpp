#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ASL_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ASL_AbsoluteX_Test, ASL_AbsoluteX) {
    // Addr = 0x4480 + 1 = 0x4481
    // Mem[0x4481] = 0x01 -> 0x02
    cpu.X = 0x01;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ASL_ABSX);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, 0x44);
    bus.Write(0x4481, 0x01);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x4481), 0x02);
}