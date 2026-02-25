#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ORA_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ORA_Absolute_Test, ORA_Absolute) {
    cpu.A = 0x00;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ORA_ABS);
    bus.Write(0x4001, 0x80);
    bus.Write(0x4002, 0x44);  // 0x4480
    bus.Write(0x4480, 0x0F);  // 0x00 | 0x0F = 0x0F
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}