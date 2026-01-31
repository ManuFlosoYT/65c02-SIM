#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDA_Inmediato_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_Inmediato_Test, LDA_Inmediato) {
    // Programa en memoria:
    // 0xFFFC: LDA #0xA9
    // 0xFFFD: 0x67
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    mem[0xFFFC] = INS_LDA_IM;
    mem[0xFFFD] = 0x67;
    mem[0xFFFE] = 0xFF;

    // Ciclo 1:
    //    Lee LDA #0xA9 en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta LDA #0xA9
    // Ciclo 2:
    //    Lee 0x67 en 0xFFFE
    //    PC avanza a 0xFFFF
    //    Opcode desconocido -> Retorna
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFF);
    EXPECT_EQ(cpu.A, 0x67);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Inmediato_Test, LDA_Inmediato_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_LDA_IM;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Inmediato_Test, LDA_Inmediato_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = INS_LDA_IM;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}
