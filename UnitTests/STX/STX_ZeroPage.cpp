#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class STX_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(STX_ZeroPage_Test, STX_ZeroPage) {
    cpu.X = 0x37;

    // 0xFFFC: STX (ZeroPage) 0x42
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_STX_ZP);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_JAM);  // Stop

    bus.Write(0x0042, 0x00);

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x0042), 0x37);
    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}