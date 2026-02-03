#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDA_ZeroPage_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_ZeroPage_Test, LDA_ZeroPage) {
    // Programa en memoria:
    // 0xFFFC: LDA (ZeroPage) 0x42
    // 0xFFFD: 0x42
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    // 0x0042: 0x37 (Valor a cargar)
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ZP;
    mem[0x4001] = 0x42;
    mem[0x0042] = 0x37;
    mem[0x4002] = INS_JAM;

    // Ciclo 1:
    //    Lee LDA (ZP) en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta LDA (ZP)
    // Ciclo 2:
    //    Lee la dirección ZP (0x42) en 0xFFFD
    //    PC avanza a 0xFFFE
    // Ciclo 3:
    //    Lee el valor (0x37) en 0x0042
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_ZeroPage_Test, LDA_ZeroPage_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ZP;
    mem[0x4001] = 0x42;
    mem[0x0042] = 0x00;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_ZeroPage_Test, LDA_ZeroPage_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ZP;
    mem[0x4001] = 0x42;
    mem[0x0042] = 0xFF;
    mem[0x4002] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0xFF);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}