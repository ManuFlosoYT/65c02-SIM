#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/Mem.h"
#include "../../Hardware/CPU/ListaInstrucciones.h"

class Ampliados_Fibonacci : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

TEST_F(Ampliados_Fibonacci, Serie_Fibonacci) {
    // Programa: Fibonacci (8 iteraciones)
    // 0, 1, 1, 2, 3, 5, 8, 13, 21, 34
    // Usaremos un buffer en memoria desde 0x00 para guardar la serie.

    Byte N = 10;  // Calcular 10 numeros
    Word ADDR_SERIES = 0x0000;

    // Inicializacion manual de los dos primeros numeros
    mem.Write(ADDR_SERIES, 0);
    mem.Write(ADDR_SERIES + 1, 1);

    // Codigo en 0x1000
    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    // LDX #$00 (Indice para leer F[i])
    mem.Write(PC++, INS_LDX_IM);
    mem.Write(PC++, 0x00);

    // LDY #$08 (Contador de iteraciones: N - 2)
    mem.Write(PC++, INS_LDY_IM);
    mem.Write(PC++, N - 2);

    Word LOOP_START = PC;
    // Bucle:
    // CLC
    mem.Write(PC++, INS_CLC);

    // LDA ADDR_SERIES, X (Cargar F[i])
    mem.Write(PC++, INS_LDA_ZPX);
    mem.Write(PC++, (Byte)ADDR_SERIES);

    // INX (Apuntar a F[i+1])
    mem.Write(PC++, INS_INX);

    // ADC ADDR_SERIES, X (Sumar F[i+1])
    mem.Write(PC++, INS_ADC_ZPX);
    mem.Write(PC++, (Byte)ADDR_SERIES);

    // Dejar X apuntando al siguiente (F[i+2]) para guardar
    // INX
    mem.Write(PC++, INS_INX);

    // STA ADDR_SERIES, X (Guardar F[i+2])
    mem.Write(PC++, INS_STA_ZPX);
    mem.Write(PC++, (Byte)ADDR_SERIES);

    // Retroceder X para la siguiente iteracion.
    // En la siguiente iteracion queremos leer F[i+1] y F[i+2].
    // Actualmente X apunta a F[i+2]. Debemos decrementar X para que apunte a
    // F[i+1] que sera el nuevo F[i] del siguiente loop. DEX
    mem.Write(PC++, INS_DEX);

    // DEY (Decrementar contador de iteraciones)
    mem.Write(PC++, INS_DEY);

    // BNE LOOP_START
    mem.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    mem.Write(PC++, (Byte)offset);

    // Stop
    mem.Write(PC++, INS_JAM);

    // Execute
    cpu.PC = CODE_START;
    cpu.isInit = true;  // Prevenir Reset
    // Para evitar loops infinitos si falla BNE, ponemos limite de ciclos
    mem.Write(0xFFFC, 0x00);
    mem.Write(0xFFFD, 0x40);
    cpu.Execute(mem);

    // Verificaciones
    Byte expected[] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
    for (int i = 0; i < N; i++) {
        EXPECT_EQ(mem[ADDR_SERIES + i], expected[i]) << "Error en indice " << i;
    }
}