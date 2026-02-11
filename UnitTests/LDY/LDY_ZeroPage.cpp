#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class LDY_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDY_ZeroPage_Test, LDY_ZeroPage) {
    // 0xFFFC: LDY 0x42
    // 0xFFFD: 0x42
    // 0x0042: 0x37
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x37);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.Y, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_ZeroPage_Test, LDY_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.Y = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDY_ZeroPage_Test, LDY_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.Y = 0x00;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDY_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x80);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.Y, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}