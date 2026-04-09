#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class PHA_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(PHA_Test, PHA) {
    // 0xFFFC: PHA
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_PHA);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.A = 0x42;

    cpu.Execute(bus);

    EXPECT_EQ(bus.ReadDirect(0x01FF), 0x42);
    EXPECT_EQ(cpu.SP, 0x01FE);
}