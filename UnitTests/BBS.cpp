#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BBS_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// BBSx: Branch if bit x of Accumulator is Set (1).
// 2-byte instruction: Opcode, Offset.
// Address Mode: Relative

TEST_F(BBS_Test, BBS0_BranchTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBS0);
    bus.Write(0x4001, 0x04);     // Offset +4
    bus.Write(0x4002, INS_JAM);  // Stop if not taken

    // Accumulator Value: Bit 0 is 1
    cpu.A = 0x01;  // 0000 0001

    // Target: PC (after fetch offset at 0xFFFD -> 0xFFFE) + 4 = 0x0002
    bus.Write(0x4006, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4007);  // 0x0002 + 1 (Stop fetch) = 0x0003
}

TEST_F(BBS_Test, BBS0_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBS0);
    bus.Write(0x4001, 0x04);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Accumulator Value: Bit 0 is 0
    cpu.A = 0xFE;  // 1111 1110

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}

TEST_F(BBS_Test, BBS7_BranchTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBS7);
    bus.Write(0x4001, 0x05);     // Offset +5
    bus.Write(0x4002, INS_JAM);  // Stop if not taken

    // Accumulator Value: Bit 7 is 1
    cpu.A = 0x80;  // 1000 0000

    // Target: PC (0xFFFE) + 5 = 0x0003
    bus.Write(0x4007, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4008);  // 0x0003 + 1 (Stop fetch) = 0x0004
}

TEST_F(BBS_Test, BBS7_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBS7);
    bus.Write(0x4001, 0x05);
    bus.Write(0x4002, INS_JAM);  // Stop

    // Accumulator Value: Bit 7 is 0
    cpu.A = 0x7F;  // 0111 1111

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4003);  // 0xFFFE + 1 (Stop fetch) = 0xFFFF
}