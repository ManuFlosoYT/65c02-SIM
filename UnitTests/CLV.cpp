#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/Mem.h"
#include "../Hardware/CPU/ListaInstrucciones.h"

class CLV_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLV_Test, CLV_ClearsOverflow) {
    cpu.V = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_CLV);
    mem.Write(0x4001, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.V);
}