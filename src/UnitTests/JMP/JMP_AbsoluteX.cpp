#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class JMP_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(JMP_AbsoluteX_Test, JMP_AbsoluteX) {
    // 0xFFFC: JMP (0x8000, X)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // X = 4
    // Target = 0x8000 + 4 = 0x8004
    // 0x8004: 0xFF (Stop)

    cpu.X = 0x04;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_JMP_ABSX);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);

    bus.WriteDirect(0x8004, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x8005);
}