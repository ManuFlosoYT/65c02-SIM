#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDX_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDX_Inmediato_Test, LDX_Inmediato) {
    // Programa en memoria:
    // 0xFFFC: LDX #0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    mem[0xFFFC] = INS_LDX_IM;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFF);
    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Inmediato_Test, LDX_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.X = 0xFF;

    mem[0xFFFC] = INS_LDX_IM;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDX_Inmediato_Test, LDX_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.X = 0x00;

    mem[0xFFFC] = INS_LDX_IM;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.X, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
