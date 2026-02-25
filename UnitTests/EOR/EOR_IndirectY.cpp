#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class EOR_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(EOR_IndirectY_Test, EOR_IndirectY) {
    cpu.Y = 0x04;
    cpu.A = 0xFF;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_EOR_INDY);
    bus.Write(0x4001, 0x02);     // ZP
    bus.Write(0x0002, 0x00);     // Low Byte
    bus.Write(0x0003, 0x80);     // High Byte -> Base: 0x8000
    bus.WriteDirect(0x8004, 0x0F);  // 0x8000 + 0x04. 0xFF ^ 0x0F = 0xF0
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}