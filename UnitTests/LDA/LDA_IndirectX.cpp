#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LDA_IndirectX_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_IndirectX_Test, LDA_IndirectX) {
    cpu.X = 0x04;
    // Programa en memoria:
    // 0xFFFC: LDA (IndirectX) 0x02
    // 0xFFFD: 0x02 (Zero Page Address)
    // 0xFFFE: Opcode desconocido
    // X register: 0x04
    // Dirección ZP efectiva: 0x02 + 0x04 = 0x06
    // 0x0006: 0x00 (Low Byte Address)
    // 0x0007: 0x80 (High Byte Address)
    // Dirección final: 0x8000
    // 0x8000: 0x37 (Valor a cargar)
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDX);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0006, 0x00);
    mem.Write(0x0007, 0x80);
    mem.Write(0x8000, 0x37);
    mem.Write(0x4002, INS_JAM);

    // Ciclo 1:
    //    Lee LDA (INDX) en 0xFFFC -> PC=FFFD
    //    Ejecuta LDA (INDX)
    // Ciclo 2:
    //    Lee ZP Address (0x02) en 0xFFFD -> PC=FFFE
    // Ciclo 3:
    //    Lee ZP Address (0x02)
    //    Calcula ZP efectiva (0x02 + X)
    // Ciclo 4:
    //    Lee Low Byte Address en 0x0006 (0x00)
    // Ciclo 5:
    //    Lee High Byte Address en 0x0007 (0x80)
    //    Dirección efectiva: 0x8000
    // Ciclo 6:
    //    Lee valor (0x37) en 0x8000
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_ZeroFlag) {
    cpu.X = 0x04;
    cpu.A = 0xFF;
    cpu.Z = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDX);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0006, 0x00);
    mem.Write(0x0007, 0x80);
    mem.Write(0x8000, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_NegativeFlag) {
    cpu.X = 0x04;
    cpu.N = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDX);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0006, 0x00);
    mem.Write(0x0007, 0x80);
    mem.Write(0x8000, 0x88);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x88);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_IndirectX_Test, LDA_IndirectX_Wrapping) {
    cpu.X = 0x02;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDX);
    mem.Write(0x4001, 0xFF);

    mem.Write(0x0001, 0x34);
    mem.Write(0x0002, 0x12);

    mem.Write(0x1234, 0x99);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x99);
}