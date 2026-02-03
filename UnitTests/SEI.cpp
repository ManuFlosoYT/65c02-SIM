#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class SEI_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(SEI_Test, SEI_SetsInterruptDisable) {
    cpu.I = 0;

    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    mem.Write(0x4000, INS_SEI);
    mem.Write(0x4001, INS_JAM);

    cpu.Ejecutar(mem);

    EXPECT_TRUE(cpu.I);
}