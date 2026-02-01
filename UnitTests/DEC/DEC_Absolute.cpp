#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class DEC_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(DEC_Absolute_Test, DEC_Absolute) {
    // Addr = 0x4480
    // Mem[0x4480] = 0x05 -> 0x04
    mem[0xFFFC] = INS_DEC_ABS;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4480] = 0x05;
    mem[0xFFFF] = 0xFF;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4480], 0x04);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}
