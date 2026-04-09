#include <gtest/gtest.h>

#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class Ampliados_BloqueMemoria : public ::testing::Test {
protected:
    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }
    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Ampliados_BloqueMemoria, Copia_Bloque) {
    // Programa: Copiar bloque de memoria de Page 0x80 a Page 0x90
    // Longitud: 0xFF (255 bytes)

    Word SRC_PAGE = 0x4000;
    Word DST_PAGE = 0x2000;
    Word CODE_START = 0x1000;

    // Init memoria origen con valores
    for (int i = 0; i < 256; i++) {
        bus.Write(SRC_PAGE + i, (Byte)i);
        bus.Write(DST_PAGE + i, 0);  // Limpiar destino
    }

    Word PC = CODE_START;

    // LDX #$00 (Indice)
    bus.Write(PC++, INS_LDX_IM);
    bus.Write(PC++, 0x00);

    Word LOOP_START = PC;
    // Bucle:

    // LDA SRC_PAGE, X
    // Nota: Como es Absoluto indexed X, toma 2 bytes de direccion.
    // Usaremos direccion base SRC_PAGE (0x8000)
    bus.Write(PC++, INS_LDA_ABSX);
    bus.Write(PC++, (Byte)(SRC_PAGE & 0xFF));  // Low byte
    bus.Write(PC++, (Byte)(SRC_PAGE >> 8));    // High byte

    // STA DST_PAGE, X
    bus.Write(PC++, INS_STA_ABSX);
    bus.Write(PC++, (Byte)(DST_PAGE & 0xFF));  // Low byte
    bus.Write(PC++, (Byte)(DST_PAGE >> 8));    // High byte

    // INX
    bus.Write(PC++, INS_INX);

    // BNE LOOP_START
    // INX incrementa y, al desbordar de 255 a 0, Z flag se pondra a 1.
    // Eso ocurrira DESPUES de copiar el byte en indice 255 (0xFF).
    // Cuando X pasa de 0xFF a 0x00, Z=1, y BNE no salta.
    bus.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    bus.Write(PC++, (Byte)offset);

    // Stop
    bus.Write(PC++, INS_JAM);

    // Execute
    cpu.PC = CODE_START;
    cpu.isInit =
        true;  // Previene ejecutar de resetear PC a vector 0xFFFC (que es 0)
    cpu.Execute(bus);

    // Verificacion
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(bus.ReadDirect(DST_PAGE + i), (Byte)i) << "Fallo en indice " << i;
    }

    EXPECT_EQ(cpu.X, 0x00);  // Debe haber dado la vuelta completa
}