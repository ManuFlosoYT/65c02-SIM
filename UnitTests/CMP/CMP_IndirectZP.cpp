#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class CMP_IndirectZP_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Equal) {
    cpu.A = 0x10;
    cpu.C = 0;  // Pre-condition check

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IND_ZP);
    mem.Write(0x4001, 0x05);     // ZP Addr
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0005, 0x00);
    mem.Write(0x0006, 0x80);     // Pointer -> 0x8000
    mem.WriteROM(0x8000, 0x10);  // Match A

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Greater) {
    cpu.A = 0x20;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IND_ZP);
    mem.Write(0x4001, 0x10);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0010, 0x00);
    mem.Write(0x0011, 0x90);     // Pointer -> 0x9000
    mem.WriteROM(0x9000, 0x10);  // A > M (0x20 > 0x10)

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x20);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.C);  // A >= M
    EXPECT_FALSE(cpu.N);
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_Less) {
    cpu.A = 0x10;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IND_ZP);
    mem.Write(0x4001, 0x20);
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x0020, 0x00);
    mem.Write(0x0021, 0xA0);     // Pointer -> 0xA000
    mem.WriteROM(0xA000, 0x20);  // A < M (0x10 < 0x20)

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x10);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.C);  // A < M
    EXPECT_TRUE(cpu.N);   // Negative result (approx 16 - 32)
}

TEST_F(CMP_IndirectZP_Test, CMP_IndirectZP_PointerWrap) {  // ZP = 0xFF
    cpu.A = 0x50;

    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_IND_ZP);
    mem.Write(0x4001, 0xFF);     // ZP Boundary
    mem.Write(0x4002, INS_JAM);  // Stop
    mem.Write(0x00FF, 0x10);     // Low
    mem.Write(0x0000, 0xB0);     // High (wrapped) -> 0xB010
    mem.WriteROM(0xB010, 0x50);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x50);
    EXPECT_TRUE(cpu.Z);  // Equal
    EXPECT_TRUE(cpu.C);
}