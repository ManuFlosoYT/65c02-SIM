#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class CMP_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CMP_ZeroPage_Test, CMP_ZeroPage) {
    // A = 5, Mem = 5
    cpu.A = 0x05;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CMP_ZP);
    mem.Write(0x4001, 0x42);
    mem.Write(0x0042, 0x05);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.C);
}