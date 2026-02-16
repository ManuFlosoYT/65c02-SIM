#include <gtest/gtest.h>

#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class PruebaNOP : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(PruebaNOP, NoHaceNada) {
    // Programa en memoria:
    // 0xFFFC: NOP (0xEA)
    // 0xFFFD: Opcode desconocido (0xFF) para detener la ejecución
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_NOP);
    mem.Write(0x4001, INS_JAM);

    // Ciclo 1:
    //     Lee NOP en 0xFFFC
    //     PC avanza a 0xFFFD
    //     Ejecuta NOP
    // Ciclo 2:
    //     Lee 0xFF en 0xFFFD
    //     PC avanza a 0xFFFE
    //     Opcode desconocido
    //     Retorna
    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4002);

    // Verify Registers are unchanged (Default 0 in Setup, but let's change them
    // before)
}

TEST_F(PruebaNOP, NoModificaEstado) {
    cpu.A = 0x42;
    cpu.X = 0x42;
    cpu.Y = 0x42;
    cpu.SP = 0xF0;
    cpu.C = 1;
    cpu.Z = 1;
    cpu.I = 1;
    cpu.D = 1;
    cpu.B = 1;
    cpu.V = 1;
    cpu.N = 1;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_NOP);
    mem.Write(0x4001, INS_JAM);

    cpu.Execute(mem);

    EXPECT_EQ(cpu.PC, 0x4002);
    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_EQ(cpu.X, 0x42);
    EXPECT_EQ(cpu.Y, 0x42);
    EXPECT_EQ(cpu.SP, 0xF0);
    EXPECT_TRUE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_TRUE(cpu.I);
    EXPECT_TRUE(cpu.D);
    EXPECT_TRUE(cpu.B);
    EXPECT_TRUE(cpu.V);
    EXPECT_TRUE(cpu.N);
}