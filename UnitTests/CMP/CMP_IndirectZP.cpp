#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class CMP_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Equal) {
    cpu.A = 0x10;
    cpu.C = 0;  // Pre-condition check

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x05);     // ZP Addr
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0005, 0x00);
    bus.Write(0x0006, 0x80);     // Pointer -> 0x8000
    bus.WriteDirect(0x8000, 0x10);  // Match A

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Greater) {
    cpu.A = 0x20;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x10);
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0010, 0x00);
    bus.Write(0x0011, 0x90);     // Pointer -> 0x9000
    bus.WriteDirect(0x9000, 0x10);  // A > M (0x20 > 0x10)

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x20);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Less) {
    cpu.A = 0x10;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0x20);
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x0020, 0x00);
    bus.Write(0x0021, 0xA0);     // Pointer -> 0xA000
    bus.WriteDirect(0xA000, 0x20);  // A < M (0x10 < 0x20)

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);  // A < M
    EXPECT_TRUE(cpu.N);   // Negative result (approx 16 - 32)
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x50;

    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    bus.Write(0x4000, INS_CMP_IND_ZP);
    bus.Write(0x4001, 0xFF);     // ZP Boundary
    bus.Write(0x4002, INS_JAM);  // Stop
    bus.Write(0x00FF, 0x10);     // Low
    bus.Write(0x0000, 0xB0);     // High (wrapped) -> 0xB010
    bus.WriteDirect(0xB010, 0x50);

    cpu.Execute(bus);

    EXPECT_EQ(cpu.A, 0x50);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);
}