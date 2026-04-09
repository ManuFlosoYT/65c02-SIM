#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class LDA_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP) {
    // 0xFFFC: LDA (ZP)
    // 0xFFFD: 0x20 (Pointer address in ZP)
    // 0x0020: 0x00 (Low byte of target)
    // 0x0021: 0x80 (High byte of target) -> Target Address: 0x8000
    // 0x8000: 0x37 (Value to load)

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0x80);
    bus.WriteDirect(0x8000, 0x37);
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_ZeroFlag) {
    cpu.A = 0xFF;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0x80);
    bus.WriteDirect(0x8000, 0x00);  // Load 0x00
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_NegativeFlag) {
    cpu.A = 0x00;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0x80);
    bus.WriteDirect(0x8000, 0x80);  // Load 0x80 (Negative)
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_IndirectZP_Test, LDA_IndirectZP_WrapAround) {
    // Test the ZP wrap-around behavior explicitly handled in LDA implementation
    // Pointer at 0xFF.
    // Low byte at 0xFF.
    // High byte at 0x00 (Wrap around).

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_LDA_IND_ZP);
    bus.Write(0x4001, 0xFF);
    bus.Write(0x00FF, 0x00);  // Low byte of target address
    bus.Write(0x0000, 0x90);  // High byte of target address -> Target: 0x9000
    bus.WriteDirect(0x9000, 0x42);  // Value to load
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}