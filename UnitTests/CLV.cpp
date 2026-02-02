#include <gtest/gtest.h>

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"

class CLV_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }

    Mem mem;
    CPU cpu;
};

TEST_F(CLV_Test, CLV_ClearsOverflow) {
    cpu.V = 1;

    mem[0xFFFC] = INS_CLV;
    mem[0xFFFD] = INS_JAM;

    cpu.Ejecutar(mem);

    EXPECT_FALSE(cpu.V);
}
