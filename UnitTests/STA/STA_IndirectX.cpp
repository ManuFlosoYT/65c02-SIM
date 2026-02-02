#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STA_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_IndirectX_Test, STA_IndirectX) {
    cpu.A = 0x37;
    cpu.X = 0x04;

    // 0xFFFC: STA (IndirectX) 0x20
    mem[0xFFFC] = INS_STA_INDX;
    mem[0xFFFD] = 0x20;
    mem[0xFFFE] = INS_JAM;  // Stop

    // Pointer Address = 0x20 + 0x04 = 0x24 (ZP)
    mem[0x0024] = 0x74;  // Low Byte
    mem[0x0025] = 0x20;  // High Byte

    // Effective Address = 0x2074
    mem[0x2074] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x2074], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
