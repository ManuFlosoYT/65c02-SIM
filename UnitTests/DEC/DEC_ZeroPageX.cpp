#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class DEC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_ZeroPageX_Test, DEC_ZeroPageX) {
    // Addr = 0x42 + 0x05 = 0x47
    // Mem[0x47] = 0x05 -> 0x04
    cpu.X = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x0047], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(DEC_ZeroPageX_Test, DEC_ZeroPageX_WrapAround) {
    // Addr = 0x80 + 0xFF = 0x17F -> 0x7F
    // Mem[0x7F] = 0x05 -> 0x04
    cpu.X = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x007F, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x007F], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}