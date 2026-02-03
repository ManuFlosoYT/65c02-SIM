#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LDA_AbsoluteX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX) {
    cpu.X = 0x01;
    // Programa en memoria:
    // 0xFFFC: LDA (AbsoluteX) 0x4480
    // 0xFFFD: 0x80 (Low Byte)
    // 0xFFFE: 0x44 (High Byte)
    // 0xFFFF: Opcode desconocido
    // X register: 0x01
    // Dirección: 0x4480 + 0x01 = 0x4481
    // 0x4481: 0x37 (Valor a cargar)
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ABSX;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x37;
    mem[0x4003] = INS_JAM;

    // Ciclo 1:
    //    Lee LDA (ABSX) en 0xFFFC -> PC=FFFD
    //    Ejecuta LDA (ABSX)
    // Ciclo 2:
    //    Lee Low Byte (0x80) en 0xFFFD -> PC=FFFE
    // Ciclo 3:
    //    Lee High Byte (0x44) en 0xFFFE -> PC=FFFF
    // Ciclo 4:
    //    Lee valor (0x37) en 0x4481 (0x4480 + 0x01)
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX_ZeroFlag) {
    cpu.X = 0x01;
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ABSX;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x00;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteX_Test, LDA_AbsoluteX_NegativeFlag) {
    cpu.X = 0x01;
    cpu.N = 0;

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_LDA_ABSX;
    mem[0x4001] = 0x80;
    mem[0x4002] = 0x44;
    mem[0x4481] = 0x81;
    mem[0x4003] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x81);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}