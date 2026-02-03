#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class BMI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

// BMI: Branch if N = 1

TEST_F(BMI_Test, BMI_NoBranch_NegativeClear) {
    cpu.N = 0;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BMI);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1002, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1003);
}

TEST_F(BMI_Test, BMI_Branch_NegativeSet) {
    cpu.N = 1;
    cpu.PC = 0x1000;

    mem.Write(0x1000, INS_BMI);
    mem.Write(0x1001, 0x05);
    mem.Write(0x1007, INS_JAM);

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x10);
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0x1008);
}