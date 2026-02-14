#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

using namespace Hardware;

class LDA_IndirectY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_IndirectY_Test, LDA_IndirectY) {
    // Programa en memoria:
    // 0xFFFC: LDA (IndirectY) 0x02
    // 0xFFFD: 0x02 (Zero Page Address)
    // 0xFFFE: Opcode desconocido
    // Y register: 0x01
    // Dirección ZP: 0x02
    // 0x0002: 0x00 (Low Byte)
    // 0x0003: 0x80 (High Byte)
    // Dirección base leída: 0x8000
    // Dirección efectiva: 0x8000 + Y (0x01) = 0x8001
    // 0x8001: 0x37 (Valor a cargar)
    cpu.Y = 0x01;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDY);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0002, 0x00);
    mem.Write(0x0003, 0x80);
    mem.Write(0x8001, 0x37);
    mem.Write(0x4002, INS_JAM);

    // Ciclo 1:
    //    Lee LDA (INDY) en 0xFFFC -> PC=FFFD
    //    Ejecuta LDA (INDY)
    // Ciclo 2:
    //    Lee ZP Address (0x02) en 0xFFFD -> PC=FFFE
    // Ciclo 3:
    //    Lee Low Byte Address (0x00) en 0x0002
    // Ciclo 4:
    //    Lee High Byte Address (0x80) en 0x0003
    //    Dirección base: 0x8000
    //    Calcula Dirección efectiva: 0x8000 + Y (0x01) = 0x8001
    // Ciclo 5
    //    1. Opcode
    //    2. ZP Addr
    //    3. Read ZP (Low)
    //    4. Read ZP+1 (High) + Add Y to Low
    //    5. Read Effective Address
    //    Si, 5 ciclos minimo.
    //    Lee valor (0x37) en 0x8001
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectY_Test, LDA_IndirectY_ZeroFlag) {
    cpu.Y = 0x01;
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDY);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0002, 0x00);
    mem.Write(0x0003, 0x80);
    mem.Write(0x8001, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_IndirectY_Test, LDA_IndirectY_NegativeFlag) {
    cpu.Y = 0x01;
    cpu.N = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDY);
    mem.Write(0x4001, 0x02);
    mem.Write(0x0002, 0x00);
    mem.Write(0x0003, 0x80);
    mem.Write(0x8001, 0xAA);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0xAA);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}

TEST_F(LDA_IndirectY_Test, LDA_IndirectY_PageCrossing) {
    cpu.Y = 0x10;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDY);
    mem.Write(0x4001, 0x02);

    mem.Write(0x0002, 0xF0);
    mem.Write(0x0003, 0x10);

    mem.Write(0x1100, 0x55);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x55);
}

TEST_F(LDA_IndirectY_Test, LDA_IndirectY_PointerWrapping) {
    cpu.Y = 0x00;
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_INDY);
    mem.Write(0x4001, 0xFF);

    mem.Write(0x00FF, 0x34);
    mem.Write(0x0000, 0x12);

    mem.Write(0x1234, 0x88);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x88);
}