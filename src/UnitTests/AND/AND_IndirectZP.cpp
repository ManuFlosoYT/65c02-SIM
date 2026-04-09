#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class AND_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(AND_IndirectZP_Test, AND_IndirectZP) {
    // INS_AND_IND_ZP
    // Operand: Byte (ZP Address of Pointer)
    // Effective Address = Mem[ZP] | (Mem[ZP + 1] << 8)
    // No indexing
    cpu.A = 0xFF;
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_AND_IND_ZP);
    bus.Write(0x4001, 0x20);     // ZP Pointer
    bus.Write(0x0020, 0x00);     // Low Byte
    bus.Write(0x0021, 0x80);     // High Byte -> Target Base: 0x8000
    bus.WriteDirect(0x8000, 0x37);  // Target Value
    bus.Write(0x4002, INS_JAM);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}