#include <gtest/gtest.h>

#include "Hardware/Core/Bus.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class TXS_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(TXS_Test, TXS) {
    // 0xFFFC: TXS
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_TXS);
    bus.Write(0x4001, INS_JAM);  // Stop

    cpu.X = 0x80;
    cpu.SP = 0x01FF;  // Default

    // TXS copies X to SP (Low byte). High byte fixed at 0x01?
    // "Transfer X to Stack Pointer".
    // 6502 behavior: SP = X. High byte is implied 0x01.
    // In this emu, SP is Word.
    // If the implementation follows standard 6502, SP should become 0x0180.

    cpu.Execute(bus);

    EXPECT_EQ(cpu.SP, 0x0180);
    // Flags not affected
}