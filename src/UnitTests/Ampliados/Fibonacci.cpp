#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Ampliados_Fibonacci : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Ampliados_Fibonacci, Serie_Fibonacci) {
    // Programa: Fibonacci (8 iteraciones)
    // 0, 1, 1, 2, 3, 5, 8, 13, 21, 34
    // Usaremos un buffer en memoria desde 0x00 para guardar la serie.

    Byte N = 10;  // Calcular 10 numeros
    Word ADDR_SERIES = 0x0000;

    // Inicializacion manual de los dos primeros numeros
    bus.Write(ADDR_SERIES, 0);
    bus.Write(ADDR_SERIES + 1, 1);

    // Codigo en 0x1000
    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    // LDX #$00 (Indice para leer F[i])
    bus.Write(PC++, INS_LDX_IM);
    bus.Write(PC++, 0x00);

    // LDY #$08 (Contador de iteraciones: N - 2)
    bus.Write(PC++, INS_LDY_IM);
    bus.Write(PC++, N - 2);

    Word LOOP_START = PC;
    // Bucle:
    // CLC
    bus.Write(PC++, INS_CLC);

    // LDA ADDR_SERIES, X (Cargar F[i])
    bus.Write(PC++, INS_LDA_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    // INX (Apuntar a F[i+1])
    bus.Write(PC++, INS_INX);

    // ADC ADDR_SERIES, X (Sumar F[i+1])
    bus.Write(PC++, INS_ADC_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    // Dejar X apuntando al siguiente (F[i+2]) para guardar
    // INX
    bus.Write(PC++, INS_INX);

    // STA ADDR_SERIES, X (Guardar F[i+2])
    bus.Write(PC++, INS_STA_ZPX);
    bus.Write(PC++, (Byte)ADDR_SERIES);

    // Retroceder X para la siguiente iteracion.
    // En la siguiente iteracion queremos leer F[i+1] y F[i+2].
    // Actualmente X apunta a F[i+2]. Debemos decrementar X para que apunte a
    // F[i+1] que sera el nuevo F[i] del siguiente loop. DEX
    bus.Write(PC++, INS_DEX);

    // DEY (Decrementar contador de iteraciones)
    bus.Write(PC++, INS_DEY);

    // BNE LOOP_START
    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    // Stop
    bus.Write(PC++, INS_JAM);

    // Execute
    cpu.PC = CODE_START;
    cpu.isInit = true;  // Prevenir Reset
    // Para evitar loops infinitos si falla BNE, ponemos limite de ciclos
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    cpu.Execute(bus);

    // Verificaciones
    Byte expected[] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
    for (int i = 0; i < N; i++) {
        EXPECT_EQ(bus.ReadDirect(ADDR_SERIES + i), expected[i]) << "Error en indice " << i;
    }
}