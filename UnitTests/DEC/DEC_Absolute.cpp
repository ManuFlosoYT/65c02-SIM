#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class DEC_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_Absolute_Test, DEC_Absolute) {
    // Addr = 0x4480
    // Mem[0x4480] = 0x05 -> 0x04
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_DEC_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x05);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(mem[0x4480], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}