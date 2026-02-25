#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STA_IndirectY_Test, STA_IndirectY) {
    cpu.A = 0x37;
    cpu.Y = 0x10;

    // 0xFFFC: STA (IndirectY) 0x20
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_INDY);
    bus.Write(0x4001, 0x20);     // Zero Page
    bus.Write(0x4002, INS_JAM);  // Stop

    // Pointer at 0x20 (ZP) -> 0x2070
    bus.Write(0x0020, 0x70);  // Low
    bus.Write(0x0021, 0x20);  // High

    // Effective Address = 0x2070 + 0x10 = 0x2080
    bus.Write(0x2080, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x2080), 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}