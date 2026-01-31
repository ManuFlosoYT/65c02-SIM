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

    cpu.Reset(mem);

    // Programa en memoria:
    // 0xFFFC: NOP (0xEA)
    // 0xFFFD: Opcode desconocido (0xFF) para detener la ejecución
    mem[0xFFFC] = INS_NOP;
    mem[0xFFFD] = 0xFF;

    // Capturar estado inicial de registros (excepto PC que cambiará)
    Word SP_Inicial = cpu.SP;
    Byte A_Inicial = cpu.A;
    Byte X_Inicial = cpu.X;
    Byte Y_Inicial = cpu.Y;

    Byte C_Inicial = cpu.C;
    Byte Z_Inicial = cpu.Z;
    Byte I_Inicial = cpu.I;
    Byte D_Inicial = cpu.D;
    Byte B_Inicial = cpu.B;
    Byte V_Inicial = cpu.V;
    Byte N_Inicial = cpu.N;

    // Ejecutar ciclo de CPU
    // Ciclo 1: Lee NOP en 0xFFFC -> PC avanza a 0xFFFD -> Ejecuta NOP
    // Ciclo 2: Lee 0xFF en 0xFFFD -> PC avanza a 0xFFFE -> Opcode desconocido -> Retorna
    cpu.Ejecutar(mem);

    // Verificar que el PC avanzó 2 bytes (1 por NOP + 1 por el opcode de parada)
    EXPECT_EQ(cpu.PC, 0xFFFE);

    // Verificar que el resto del estado permanece inalterado
    EXPECT_EQ(cpu.SP, SP_Inicial);
    
    EXPECT_EQ(cpu.A, A_Inicial);
    EXPECT_EQ(cpu.X, X_Inicial);
    EXPECT_EQ(cpu.Y, Y_Inicial);

    EXPECT_EQ(cpu.C, C_Inicial);
    EXPECT_EQ(cpu.Z, Z_Inicial);
    EXPECT_EQ(cpu.I, I_Inicial);
    EXPECT_EQ(cpu.D, D_Inicial);
    EXPECT_EQ(cpu.B, B_Inicial);
    EXPECT_EQ(cpu.V, V_Inicial);
    EXPECT_EQ(cpu.N, N_Inicial);
}
