#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class STA_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_Absolute_Test, STA_Absolute) {
    cpu.A = 0x37;

    // 0xFFFC: STA (Absolute) 0x8000
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x40);
    mem.Write(0x4000, INS_STA_ABS);
    mem.Write(0x4001, 0x00);  // Low Byte
    mem.Write(0x4002, 0x20);  // High Byte
    mem.Write(0x4003,
              INS_JAM);  // Stop? No, PC is at 0xFFFF, need to be careful.
    // STA ABS is 3 bytes.
    // 0xFFFC: Opcode
    // 0xFFFD: Low
    // 0xFFFE: High
    // 0xFFFF: Next Opcode.

    mem.Write(0x4003, INS_JAM);  // Stop

    mem.WriteROM(0x2000, 0x00);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4004);  // 0xFFFF + 1 causes wrap to 0x0000 if not
                                // handled, but FetchByte increments PC.
    // Wait, 0xFFFC -> 0xFFFD (Fetch Opcode)
    // 0xFFFD -> 0xFFFE (Fetch Low)
    // 0xFFFE -> 0xFFFF (Fetch High)
    // Next Opcode at 0xFFFF.
    // 0xFFFF -> 0x0000 (Fetch Opcode 0xFF)

    EXPECT_EQ(mem[0x2000], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}