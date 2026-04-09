#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STA_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STA_AbsoluteX_Test, STA_AbsoluteX) {
    cpu.A = 0x37;
    cpu.X = 0x10;

    // 0xFFFC: STA (AbsoluteX) 0x2000
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x20);
    bus.Write(0x4003, INS_JAM);  // Stop

    // Target = 0x2000 + 0x10 = 0x2010
    bus.Write(0x2010, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x2010), 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}