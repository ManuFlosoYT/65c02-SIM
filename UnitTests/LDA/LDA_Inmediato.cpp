#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class LDA_Immediate_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LDA_Immediate_Test, LDA_Immediate) {
    // Programa en memoria:
    // 0xFFFC: LDA #0xA9
    // 0xFFFD: 0x67
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_IM);
    mem.Write(0x4001, 0x67);
    mem.Write(0x4002, INS_JAM);

    // Ciclo 1:
    //    Lee LDA #0xA9 en 0xFFFC
    //    PC avanza a 0xFFFD
    //    Ejecuta LDA #0xA9
    // Ciclo 2:
    //    Lee 0x67 en 0xFFFE
    //    PC avanza a 0xFFFF
    //    Opcode desconocido -> Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x67);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Immediate_Test, LDA_Immediate_ZeroFlag) {
    cpu.Z = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_IM);
    mem.Write(0x4001, 0x00);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x00);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

TEST_F(LDA_Immediate_Test, LDA_Immediate_NegativeFlag) {
    cpu.N = 0;
    cpu.A = 0xFF;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_LDA_IM);
    mem.Write(0x4001, 0x80);
    mem.Write(0x4002, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.A, 0x80);
    EXPECT_FALSE(cpu.Z);
    EXPECT_TRUE(cpu.N);
}