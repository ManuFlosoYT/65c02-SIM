#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class INC_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INC_ZeroPage_Test, INC_ZeroPage) {
    // Mem[0x42] = 0x05 -> 0x06
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_ZeroFlag) {
    // Mem[0x42] = 0xFF -> 0x00
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0xFF);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(INC_ZeroPage_Test, INC_ZeroPage_NegativeFlag) {
    // Mem[0x42] = 0x7F -> 0x80
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INC_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x7F);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0042], 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}