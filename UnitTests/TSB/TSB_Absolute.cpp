#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class TSB_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsZeroFlag) {
    // A = 0xAA (1010 1010)
    // M = 0x55 (0101 0101)
    // A & M = 0 -> Z = 1
    // M = A | M = 0xFF
    cpu.A = 0xAA;
    mem[0xFFFC] = INS_TSB_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x55;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0xFF);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_ClearsZeroFlag) {
    // A = 0x80 (1000 0000)
    // M = 0x80 (1000 0000)
    // A & M != 0 -> Z = 0
    // M = M | A = 0x80
    cpu.A = 0x80;
    mem[0xFFFC] = INS_TSB_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0x80;
    mem[0x8001] = INS_JAM;  // Instruction assumes 3 bytes. PC->8000 (Exec?) No.

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0x80);
}

TEST_F(TSB_Absolute_Test, TSB_Absolute_SetsBits) {
    // A = 0x0F (0000 1111)
    // M = 0xF0 (1111 0000)
    // Z = 1
    // M = 0xFF
    cpu.A = 0x0F;
    mem[0xFFFC] = INS_TSB_ABS;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0xF0;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.Z);
    EXPECT_EQ(mem[0x8000], 0xFF);
}
