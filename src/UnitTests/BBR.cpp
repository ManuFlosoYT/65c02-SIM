#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class BBR_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

// BBRx: Branch if bit x of Zero Page memory is Reset (0).
// 3-byte instruction: Opcode, ZeroPage, Offset.
// Address Mode: Relative

TEST_F(BBR_Test, BBR0_BranchTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBR0);
    bus.Write(0x4001, 0x10);     // ZP Address
    bus.Write(0x4002, 0x05);     // Offset +5
    bus.Write(0x4003, INS_JAM);  // Stop if not taken (or part of skip)

    // Memory Value: Bit 0 is 0
    bus.WriteDirect(0x0010, 0xFE);  // 1111 1110

    // Target: PC = 0x4003. 0x4003 + 5 = 0x4008
    bus.Write(0x4008, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4009);  // 0x4008 + 1 (Stop fetch) = 0x4009
}

TEST_F(BBR_Test, BBR0_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBR0);
    bus.Write(0x4001, 0x10);     // ZP Address
    bus.Write(0x4002, 0x05);     // Offset
    bus.Write(0x4003, INS_JAM);  // Stop if not taken

    // Memory Value: Bit 0 is 1
    bus.WriteDirect(0x0010, 0x01);  // 0000 0001 (Bit 0 set)

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004);  // 0x4003 + 1 (Stop fetch) = 0x4004
}

TEST_F(BBR_Test, BBR7_BranchTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBR7);
    bus.Write(0x4001, 0x20);     // ZP Address
    bus.Write(0x4002, 0x04);     // Offset +4
    bus.Write(0x4003, INS_JAM);  // Stop if not taken

    // Memory Value: Bit 7 is 0
    bus.WriteDirect(0x0020, 0x7F);  // 0111 1111

    // Target: PC (0x4003) + 4 = 0x4007
    bus.Write(0x4007, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4008); 
}

TEST_F(BBR_Test, BBR7_BranchNotTaken) {
    cpu.PC = 0xFFFC;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_BBR7);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, 0x04);
    bus.Write(0x4003, INS_JAM);  // Stop

    // Memory Value: Bit 7 is 1
    bus.WriteDirect(0x0020, 0x80);  // 1000 0000

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004); 
}