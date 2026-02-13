#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class ORA_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_Immediate_Test, ORA_Immediate) {
    cpu.A = 0x00;  // Start with 0x00
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_IM);
    mem.Write(0x4001, 0x0F);  // 0x00 | 0x0F = 0x0F
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ORA_Immediate_Test, ORA_Immediate_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_IM);
    mem.Write(0x4001, 0x00);  // 0x00 | 0x00 = 0x00
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ORA_Immediate_Test, ORA_Immediate_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_ORA_IM);
    mem.Write(0x4001, 0x80);  // 0x00 | 0x80 = 0x80
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}