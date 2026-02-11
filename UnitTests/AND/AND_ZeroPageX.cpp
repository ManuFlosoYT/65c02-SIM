#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class AND_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_ZeroPageX_Test, AND_ZeroPageX) {
    cpu.X = 0x05;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0047, 0x37);  // 0x42 + 0x05 = 0x47
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_ZeroPageX_Test, AND_ZeroPageX_WrapAround) {
    cpu.X = 0xFF;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_AND_ZPX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x007F, 0x37);  // 0x80 + 0xFF = 0x17F -> 0x7F (Zero Page Wrap)
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}