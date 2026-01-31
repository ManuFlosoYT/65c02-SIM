#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class PruebaJSR : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PruebaJSR, SaltaASubrutinaCorrecto) {
    mem[0xFFFC] = INS_JSR;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;
    mem[0x8000] = 0xFF;

    Word PC_Inicial = 0xFFFC;
    Word PC_RetornoEsperado = PC_Inicial + 2;
    Word SP_Inicial = cpu.SP;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x8001);
    EXPECT_EQ(cpu.SP, SP_Inicial - 2);

    Byte RetornoHigh = mem[SP_Inicial];
    Byte RetornoLow = mem[SP_Inicial - 1];

    EXPECT_EQ(RetornoHigh, (PC_RetornoEsperado >> 8) & 0xFF);
    EXPECT_EQ(RetornoLow, PC_RetornoEsperado & 0xFF);
}

TEST_F(PruebaJSR, JSRAnidado) {

    mem[0xFFFC] = INS_JSR;
    mem[0xFFFD] = 0x00;
    mem[0xFFFE] = 0x80;

    mem[0x8000] = INS_JSR;
    mem[0x8001] = 0x00;
    mem[0x8002] = 0x90;

    mem[0x9000] = 0xFF;

    Word SP_Inicial = cpu.SP;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x9001);
    EXPECT_EQ(cpu.SP, SP_Inicial - 4);

    EXPECT_EQ(mem[cpu.SP + 1], 0x02);
    EXPECT_EQ(mem[cpu.SP + 2], 0x80);
    EXPECT_EQ(mem[cpu.SP + 3], 0xFE);
    EXPECT_EQ(mem[cpu.SP + 4], 0xFF);
}
