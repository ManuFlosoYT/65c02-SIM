#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class INC_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(INC_AbsoluteX_Test, INC_AbsoluteX) {
    // Addr = 0x4480 + 0x01 = 0x4481
    // Mem[0x4481] = 0x05 -> 0x06
    cpu.X = 0x01;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_INC_ABSX);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4481, 0x05);
    mem.Write(0x4003, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4481], 0x06);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}