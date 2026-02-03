#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class AND_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(AND_Absolute_Test, AND_Absolute) {
    cpu.A = 0xFF;
    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x40;
    mem[0x4000] = INS_AND_ABS;
    mem[0x4001] = 0x80;  // Little Endian
    mem[0x4002] = 0x44;  // 0x4480
    mem[0x4480] = 0x37;
    mem[0x4003] = INS_JAM;  // Stop

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x37);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}