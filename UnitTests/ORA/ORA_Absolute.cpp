#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class ORA_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(ORA_Absolute_Test, ORA_Absolute) {
    cpu.A = 0x00;
    mem[0xFFFC] = INS_ORA_ABS;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;  // 0x4480
    mem[0x4480] = 0x0F;  // 0x00 | 0x0F = 0x0F
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.A, 0x0F);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
