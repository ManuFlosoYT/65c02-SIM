#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class SBC_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SBC_IndirectY_Test, SBC_IndirectY) {
    cpu.Y = 0x01;
    cpu.A = 0x0A;
    cpu.C = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SBC_INDY);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0002, 0x00);
    mem.Write(0x0003, 0x80);
    mem.Write(0x8001, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x05);
}