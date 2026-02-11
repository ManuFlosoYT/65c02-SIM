#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class SBC_ZeroPageX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_ZeroPageX_Test, SBC_ZeroPageX) {
    // Addr = 0x42 + 0x01 = 0x43
    // 10 - 5 = 5.
    cpu.A = 0x0A;
    cpu.X = 0x01;
    cpu.C = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_ZPX);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0043, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}