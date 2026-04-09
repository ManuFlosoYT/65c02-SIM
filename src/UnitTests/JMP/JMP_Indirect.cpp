#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class JMP_Indirect_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(JMP_Indirect_Test, JMP_Indirect) {
    // 0xFFFC: JMP (0x8000)
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0x00 (Low Byte of Target)
    // 0x8001: 0x90 (High Byte of Target) -> Target 0x9000
    // 0x9000: 0xFF (Stop)

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_JMP_IND);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);

    bus.WriteDirect(0x8000, 0x00);
    bus.WriteDirect(0x8001, 0x90);

    bus.WriteDirect(0x9000, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x9001);
}