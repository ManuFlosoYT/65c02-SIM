#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
#include "../Instrucciones/ListaInstrucciones.h"
#include <gtest/gtest.h>

class PruebaLDA : public ::testing::Test {
    protected:
        void SetUp() override { cpu.Reset(mem); }

        Mem mem;
        CPU cpu;
};

TEST_F(PruebaLDA, LDA_Inmediato) {

    cpu.Reset(mem);

    // Programa en memoria:
    // 0xFFFC: LDA #0xA9
    // 0xFFFD: 0x67
    // 0xFFFE: Opcode desconocido (0xFF) para detener la ejecución
    mem[0xFFFC] = INS_LDA_IM;
    mem[0xFFFD] = 0x67;
    mem[0xFFFE] = 0xFF;

    // Capturar estado inicial de registros (excepto PC que cambiará)
    Word SP_Inicial = cpu.SP;
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
    // Ciclo 1: Lee LDA_IM en 0xFFFC -> PC avanza a 0xFFFD -> Lee 0x67 y lo almacena en A -> PC avanza a 0xFFFE
    // Ciclo 2: Lee 0xFF en 0xFFFE -> PC avanza a 0xFFFF -> Opcode desconocido -> Retorna
    cpu.Ejecutar(mem);

    // Verificar que el PC avanzó 3 bytes (1 por LDA_IN + 1 por el dato + 1 por el opcode de parada)
    EXPECT_EQ(cpu.PC, 0xFFFF);
    EXPECT_EQ(cpu.A, 0x67);

    // Verificar que el resto del estado permanece inalterado
    EXPECT_EQ(cpu.SP, SP_Inicial);
    
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
