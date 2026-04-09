#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STA_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STA_IndirectX_Test, STA_IndirectX) {
    cpu.A = 0x37;
    cpu.X = 0x04;

    // 0xFFFC: STA (IndirectX) 0x20
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STA_INDX);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Pointer Address = 0x20 + 0x04 = 0x24 (ZP)
    bus.Write(0x0024, 0x74);  // Low Byte
    bus.Write(0x0025, 0x20);  // High Byte

    // Effective Address = 0x2074
    bus.Write(0x2074, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x2074), 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}