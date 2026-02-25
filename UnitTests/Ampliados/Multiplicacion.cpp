#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Ampliados_Multiplicacion : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Ampliados_Multiplicacion, Multiplicacion_Simple) {
    // Programa: Multiplicacion 10 * 5 = 50 (0x32)
    // 0x00: Resultado
    // 0x01: Multiplicando (10)
    // 0x02: Multiplicador (5)

    // Direcciones de memoria
    Byte ADDR_RES = 0x00;
    Byte ADDR_MUL1 = 0x01;
    Byte ADDR_MUL2 = 0x02;

    // Init valores en memoria
    bus.Write(ADDR_RES, 0x00);
    bus.Write(ADDR_MUL1, 10);
    bus.Write(ADDR_MUL2, 5);

    // Codigo en 0x1000
    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    // LDA 0x00 (Init Accumulator con 0 para el resultado)
    bus.Write(PC++, INS_LDA_IM);
    bus.Write(PC++, 0x00);

    // LDX ADDR_MUL2 (Cargar X con el multiplicador, contador del bucle)
    bus.Write(PC++, INS_LDX_ZP);
    bus.Write(PC++, ADDR_MUL2);

    // Bucle:
    // CLC (Limpiar carry antes de sumar)
    Word LOOP_START = PC;
    bus.Write(PC++, INS_CLC);

    // ADC ADDR_MUL1 (Sumar multiplicando al acumulador)
    bus.Write(PC++, INS_ADC_ZP);
    bus.Write(PC++, ADDR_MUL1);

    // DEX (Decrementar contador)
    bus.Write(PC++, INS_DEX);

    // BNE LOOP_START (Si X != 0, volver al inicio del bucle)
    // Salto relativo: PC actual esta apuntando a la siguiente instruccion.
    // BNE toma un offset signed. Destino = PC_next + offset -> offset = Destino
    // - PC_next Aqui PC ya se ha incrementado por el BNE (2 bytes). offset =
    // LOOP_START - (PC + 2)
    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    // STA ADDR_RES (Guardar resultado)
    bus.Write(PC++, INS_STA_ZP);
    bus.Write(PC++, ADDR_RES);

    // Fin (Stop)
    bus.Write(PC++, INS_JAM);  // Instruccion de parada custom en los tests

    // Execute
    cpu.PC = CODE_START;
    cpu.isInit = true;  // Previen3 Reset
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);
    cpu.Execute(bus);

    // Verificaciones
    EXPECT_EQ(bus.ReadDirect(ADDR_RES), 50);  // 10 * 5 = 50
    EXPECT_EQ(cpu.X, 0);           // Loop debe terminar en 0
}