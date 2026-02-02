#include <gtest/gtest.h>

#include "../../Componentes/CPU.h"
#include "../../Componentes/Mem.h"
#include "../../Instrucciones/ListaInstrucciones.h"

class Ampliados_Multiplicacion : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
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

    // Inicializar valores en memoria
    mem[ADDR_RES] = 0x00;
    mem[ADDR_MUL1] = 10;
    mem[ADDR_MUL2] = 5;

    // Codigo en 0x1000
    Word CODE_START = 0x1000;
    Word PC = CODE_START;

    // LDA 0x00 (Inicializar Acumulador con 0 para el resultado)
    mem[PC++] = INS_LDA_IM;
    mem[PC++] = 0x00;

    // LDX ADDR_MUL2 (Cargar X con el multiplicador, contador del bucle)
    mem[PC++] = INS_LDX_ZP;
    mem[PC++] = ADDR_MUL2;

    // Bucle:
    // CLC (Limpiar carry antes de sumar)
    Word LOOP_START = PC;
    mem[PC++] = INS_CLC;

    // ADC ADDR_MUL1 (Sumar multiplicando al acumulador)
    mem[PC++] = INS_ADC_ZP;
    mem[PC++] = ADDR_MUL1;

    // DEX (Decrementar contador)
    mem[PC++] = INS_DEX;

    // BNE LOOP_START (Si X != 0, volver al inicio del bucle)
    // Salto relativo: PC actual esta apuntando a la siguiente instruccion.
    // BNE toma un offset signed. Destino = PC_next + offset -> offset = Destino
    // - PC_next Aqui PC ya se ha incrementado por el BNE (2 bytes). offset =
    // LOOP_START - (PC + 2)
    mem[PC++] = INS_BNE;
    int8_t offset = LOOP_START - (PC + 1);
    mem[PC++] = (Byte)offset;

    // STA ADDR_RES (Guardar resultado)
    mem[PC++] = INS_STA_ZP;
    mem[PC++] = ADDR_RES;

    // Fin (Stop)
    mem[PC++] = 0xFF;  // Instruccion de parada custom en los tests

    // Ejecutar
    cpu.PC = CODE_START;
    cpu.Ejecutar(mem);

    // Verificaciones
    EXPECT_EQ(mem[ADDR_RES], 50);  // 10 * 5 = 50
    EXPECT_EQ(cpu.X, 0);           // Loop debe terminar en 0
}
