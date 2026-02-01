#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ORA_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_Inmediato_Test, ORA_Inmediato) {
    cpu.A = 0x00;  // Start with 0x00
    mem[0xFFFC] = INS_ORA_IM;
    mem[0xFFFD] = 0x0F;  // 0x00 | 0x0F = 0x0F
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ORA_Inmediato_Test, ORA_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0x00;
    mem[0xFFFC] = INS_ORA_IM;
    mem[0xFFFD] = 0x00;  // 0x00 | 0x00 = 0x00
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(ORA_Inmediato_Test, ORA_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0x00;
    mem[0xFFFC] = INS_ORA_IM;
    mem[0xFFFD] = 0x80;  // 0x00 | 0x80 = 0x80
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
