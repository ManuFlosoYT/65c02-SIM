#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDX_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDX_Absolute_Test, LDX_Absolute) {
    // 0xFFFC: LDX 0x8000
    // 0xFFFD: 0x00 (Low Byte)
    // 0xFFFE: 0x80 (High Byte)
    // 0x8000: 0x37
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x37);

    // We assume Instruction takes 3 bytes (1 opcode + 2 address)
    // Next instruction at 0xFFFF (but we are at FFFC+3 = FFFF)
    // The previous tests used 0xFFFE as the stop opcode, but if the instruction
    // is 3 bytes long: FFFC (Op), FFFD (AL), FFFE (AH) Next Opcode at FFFF.
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.PC, 0x4004);  // FFFF + 1 = 0 (wrap) or just Stop.
    // Wait, let's check LDA_ZeroPage again. It was at FFFC, FFFD (arg), FFFE
    // (stop). ZeroPage is 2 bytes. Absolute is 3 bytes. So FFFC, FFFD, FFFE.
    // Next PC is FFFF. So bus.ReadDirect(0xFFFF) = 0xFF is correct. And expected PC is
    // 0x0000 (after reading 0xFF at FFFF and returning?) The loop in
    // CPU::Execute typically runs until Break or max cycles. If 0xFF returns,
    // the PC might be incremented after fetch. Let's check LDA_ZeroPage
    // expectation: EXPECT_EQ(cpu.PC, 0x4003); Instruction at FFFC (1 byte) +
    // Arg (1 byte) = 2 bytes. Next is FFFE. Opcode at FFFE is read. PC becomes
    // FFFF. So for Absolute: Instruction at FFFC (1 byte) + Arg (2 bytes) = 3
    // bytes. PC starts FFFC -> FFFD -> FFFE -> FFFF. Next fetches Opcode at
    // FFFF. PC becomes 0x0000.

    EXPECT_EQ(cpu.X, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Absolute_Test, LDX_Absolute_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x00);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Absolute_Test, LDX_Absolute_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDX_ABS);
    bus.Write(0x4001, 0x00);
    bus.Write(0x4002, 0x80);
    bus.WriteDirect(0x8000, 0x80);
    bus.Write(0x4003, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}