#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class LSR_Absolute_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(LSR_Absolute_Test, LSR_Absolute) {
    // Mem[0x4480] = 0x02 -> 0x01
    mem[0xFFFC] = INS_LSR_ABS;
    mem[0xFFFD] = 0x80;
    mem[0xFFFE] = 0x44;
    mem[0x4480] = 0x02;
    mem[0xFFFF] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_EQ(mem[0x4480], 0x01);
}
