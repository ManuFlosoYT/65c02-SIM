#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"
#include <gtest/gtest.h>

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
    mem[0xFFFC] = INS_NOP;
    mem[0xFFFD] = 0xFF;

    // Ciclo 1: 
    //     Lee NOP en 0xFFFC
    //     PC avanza a 0xFFFD
    //     Ejecuta NOP
    // Ciclo 2: 
    //     Lee 0xFF en 0xFFFD
    //     PC avanza a 0xFFFE
    //     Opcode desconocido
    //     Retorna
    cpu.Ejecutar(mem);

    EXPECT_EQ(cpu.PC, 0xFFFE);
}
