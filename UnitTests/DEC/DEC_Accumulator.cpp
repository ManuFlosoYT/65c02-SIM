#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class DEC_Accumulator_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_Accumulator_Test, DEC_Accumulator) {
    // A = 0x05 -> A - 1 = 0x04
    cpu.A = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_A);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_Accumulator_Test, DEC_Accumulator_ZeroFlag) {
    // A = 0x01 -> A - 1 = 0x00
    cpu.A = 0x01;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_A);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_Accumulator_Test, DEC_Accumulator_NegativeFlag) {
    // A = 0x00 -> A - 1 = 0xFF
    cpu.A = 0x00;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_A);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}