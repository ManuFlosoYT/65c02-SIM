#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ROR_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(ROR_ZeroPage_Test, ROR_ZeroPage) {
    // Mem = 0x01 -> 0x00 (Shift Right, C=0 into bit 7).
    // Carry Out = Old Bit 0 (1).
    cpu.C = 0;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_ROR_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x0042, 0x01);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x00);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}