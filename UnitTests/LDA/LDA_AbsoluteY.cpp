#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LDA_AbsoluteY_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_AbsoluteY_Test, LDA_AbsoluteY) {
    cpu.Y = 0x02;
    // Programa en memoria:
    // 0xFFFC: LDA (AbsoluteY) 0x4480
    // 0xFFFD: 0x80 (Low Byte)
    // 0xFFFE: 0x44 (High Byte)
    // 0xFFFF: Opcode desconocido
    // Y register: 0x02
    // Dirección: 0x4480 + 0x02 = 0x4482
    // 0x4482: 0x37 (Valor a cargar)
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABSY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4482, 0x37);
    mem.Write(0x4003, INS_JAM);

    // Ciclo 1:
    //    Lee LDA (ABSY) en 0xFFFC -> PC=FFFD
    //    Ejecuta LDA (ABSY)
    // Ciclo 2:
    //    Lee Low Byte (0x80) en 0xFFFD -> PC=FFFE
    // Ciclo 3:
    //    Lee High Byte (0x44) en 0xFFFE -> PC=FFFF
    // Ciclo 4:
    //    Lee valor (0x37) en 0x4482
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteY_Test, LDA_AbsoluteY_ZeroFlag) {
    cpu.Y = 0x02;
    cpu.Z = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABSY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4482, 0x00);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_AbsoluteY_Test, LDA_AbsoluteY_NegativeFlag) {
    cpu.Y = 0x02;
    cpu.N = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABSY);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4482, 0x90);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x90);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}