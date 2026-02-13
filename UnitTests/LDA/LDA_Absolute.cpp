#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/InstructionSet.h"

class LDA_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_Absolute_Test, LDA_Absolute) {
    // Programa en memoria:
    // 0xFFFC: LDA (Absolute) 0x4480
    // 0xFFFD: 0x80 (Low Byte)
    // 0xFFFE: 0x44 (High Byte)
    // 0xFFFF: Opcode desconocido (0xFF)
    // 0x4480: 0x37 (Valor a cargar)
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x37);
    mem.Write(0x4003, INS_JAM);

    // Ciclo 1:
    //    Lee LDA (ABS) en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta LDA (ABS)
    // Ciclo 2:
    //    Lee Low Byte (0x80) en 0xFFFD
    //    PC avanza a 0xFFFE
    // Ciclo 3:
    //    Lee High Byte (0x44) en 0xFFFE
    //    PC avanza a 0xFFFF
    // Ciclo 4:
    //    Lee valor (0x37) en 0x4480
    //    Carga 0x37 en A
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4004);
    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Absolute_Test, LDA_Absolute_ZeroFlag) {
    cpu.A = 0xFF;
    cpu.Z = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x00);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Absolute_Test, LDA_Absolute_NegativeFlag) {
    cpu.N = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_ABS);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, 0x44);
    mem.Write(0x4480, 0x80);
    mem.Write(0x4003, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}