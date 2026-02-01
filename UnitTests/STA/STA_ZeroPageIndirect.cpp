#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class STA_ZeroPageIndirect_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(STA_ZeroPageIndirect_Test, STA_ZeroPageIndirect) {
    cpu.A = 0x37;

    // 0xFFFC: STA (ZeroPage Indirect) 0x20
    mem[0xFFFC] = INS_STA_IND_ZP;
    mem[0xFFFD] = 0x20;
    mem[0xFFFE] = 0xFF;  // Stop

    // Pointer Address (ZP) = 0x20
    mem[0x0020] = 0x00;  // Low Byte
    mem[0x0021] = 0x30;  // High Byte
    // Effective Address = 0x3000

    mem[0x3000] = 0x00;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x3000], 0x37);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
