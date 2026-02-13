#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class EOR_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(EOR_IndirectX_Test, EOR_IndirectX) {
    cpu.X = 0x04;
    cpu.A = 0xFF;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_EOR_INDX);
    mem.Write(0x4001, 0x02);  // ZP Addr = 0x02. Actual Lookup: 0x02 + 0x04 = 0x06
    mem.Write(0x0006, 0x00);  // Low Byte
    mem.Write(0x0007, 0x80);  // High Byte -> Eff Address: 0x8000
    mem.Write(0x8000, 0x0F);  // 0xFF ^ 0x0F = 0xF0
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xF0);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}