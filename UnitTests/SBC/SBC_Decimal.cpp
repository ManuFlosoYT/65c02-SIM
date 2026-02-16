#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class SBC_Decimal_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_Decimal_Test, SBC_Decimal_Simple) {
    // 0x10 - 0x05 = 0x05 (BCD)
    cpu.A = 0x10;
    cpu.D = 1;
    cpu.C = 1;  // No borrow

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_IM);
    mem.Write(0x4001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x05);
    EXPECT_TRUE(cpu.C);  // No borrow
}

TEST_F(SBC_Decimal_Test, SBC_Decimal_Borrow) {
    // 0x05 - 0x06 = 0x99 (BCD 105 - 6 = 99), C=0 (Borrow)
    cpu.A = 0x05;
    cpu.D = 1;
    cpu.C = 1;  // No borrow start

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_IM);
    mem.Write(0x4001, 0x06);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x99);
    EXPECT_FALSE(cpu.C);  // Borrow
}