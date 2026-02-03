#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_Inmediato_Test, AND_Inmediato) {
    // Programa en memoria:
    // 0xFFFC: AND #0x0F
    // 0xFFFD: 0x67
    // 0xFFFE: Opcode desconocido (0xFF)
    cpu.A = 0xFF;  // Start with A = 0xFF
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_AND_IM;
    mem[0x4001] = 0x0F;
    mem[0x4002] = INS_JAM;

    // Ciclo 1: Lee AND #0x0F
    // Ciclo 2: Lee 0x0F, ejecuta AND
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);  // 0xFF & 0x0F = 0x0F
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_Inmediato_Test, AND_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xF0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_AND_IM;
    mem[0x4001] = 0x0F;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);  // 0xF0 & 0x0F = 0x00
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(AND_Inmediato_Test, AND_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_AND_IM;
    mem[0x4001] = 0x80;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);  // 0xFF & 0x80 = 0x80
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}