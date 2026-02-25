#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class PHP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(PHP_Test, PHP) {
    // 0xFFFC: PHP
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_PHP);
    bus.Write(0x4001, INS_JAM);  // Stop

    // Set some flags
    cpu.C = 1;
    cpu.Z = 1;
    cpu.N = 1;
    cpu.V = 1;
    cpu.D = 1;
    cpu.I = 1;
    cpu.B = 1;

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x01FF), 0xDF);
    EXPECT_EQ(cpu.SP, 0x01FE);
}