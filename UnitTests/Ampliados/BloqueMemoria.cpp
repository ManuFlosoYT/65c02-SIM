#include <gtest/gtest.h>

#include "../../Hardware/CPU.h"
#include "../../Hardware/CPU/Instructions/InstructionSet.h"
#include "../../Hardware/Mem.h"

using namespace Hardware;

class Ampliados_BloqueMemoria : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
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
        mem.Write(SRC_PAGE + i, (Byte)i);
        mem.Write(DST_PAGE + i, 0);  // Limpiar destino
    }

    Word PC = CODE_START;

    // LDX #$00 (Indice)
    mem.Write(PC++, INS_LDX_IM);
    mem.Write(PC++, 0x00);

    Word LOOP_START = PC;
    // Bucle:

    // LDA SRC_PAGE, X
    // Nota: Como es Absoluto indexed X, toma 2 bytes de direccion.
    // Usaremos direccion base SRC_PAGE (0x8000)
    mem.Write(PC++, INS_LDA_ABSX);
    mem.Write(PC++, (Byte)(SRC_PAGE & 0xFF));  // Low byte
    mem.Write(PC++, (Byte)(SRC_PAGE >> 8));    // High byte

    // STA DST_PAGE, X
    mem.Write(PC++, INS_STA_ABSX);
    mem.Write(PC++, (Byte)(DST_PAGE & 0xFF));  // Low byte
    mem.Write(PC++, (Byte)(DST_PAGE >> 8));    // High byte

    // INX
    mem.Write(PC++, INS_INX);

    // BNE LOOP_START
    // INX incrementa y, al desbordar de 255 a 0, Z flag se pondra a 1.
    // Eso ocurrira DESPUES de copiar el byte en indice 255 (0xFF).
    // Cuando X pasa de 0xFF a 0x00, Z=1, y BNE no salta.
    mem.Write(PC++, INS_BNE);
    int8_t offset = LOOP_START - (PC + 1);
    mem.Write(PC++, (Byte)offset);

    // Stop
    mem.Write(PC++, INS_JAM);

    // Execute
    cpu.PC = CODE_START;
    cpu.isInit =
        true;  // Previene ejecutar de resetear PC a vector 0xFFFC (que es 0)
    cpu.Execute(mem);

    // Verificacion
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(mem[DST_PAGE + i], (Byte)i) << "Fallo en indice " << i;
    }

    EXPECT_EQ(cpu.X, 0x00);  // Debe haber dado la vuelta completa
}