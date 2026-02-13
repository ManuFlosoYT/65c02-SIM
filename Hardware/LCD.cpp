#include "LCD.h"

#include <cstring>

#include "Mem.h"

void LCD::Inicializar(Mem& mem) {
    // Reiniciar estado
    modo_cuatro_bits = false;
    esperando_nibble_bajo = false;
    ultimo_portb = 0;
    DATO_PORTB = 0;
    DATO_DDRB = 0;

    // Clear screen buffer
    std::memset(screen, ' ', sizeof(screen));
    cursorX = 0;
    cursorY = 0;

    mem.SetWriteHook(DDRB, [this](Word addr, Byte val) { DATO_DDRB = val; });
}

void LCD::Update(Byte val) {
    Byte old_val = ultimo_portb;
    ultimo_portb = val;
    DATO_PORTB = val;

    // Detectar Flanco de Bajada de E (Bit 6, 0x40)
    bool old_E = (old_val & 0x40) != 0;
    bool new_E = (val & 0x40) != 0;

    if (old_E && !new_E) {
        // E pasó de Alto -> Bajo. Latchear Datos.
        Byte data_nibble = val & 0x0F;  // PB0-3 -> D4-D7
        bool rs = (val & 0x10) != 0;    // PB4 = RS
        bool rw = (val & 0x20) != 0;    // PB5 = RW

        // Solo nos importan las Escrituras (RW=0)
        if (rw) return;

        if (!modo_cuatro_bits) {
            // Detección de modo 8-bits
            if (!rs && data_nibble == 0x02) {
                modo_cuatro_bits = true;
                esperando_nibble_bajo = false;
            }
        } else {
            // Modo 4-bits
            if (!esperando_nibble_bajo) {
                // Capturar Nibble Alto
                nibble_alto_actual = data_nibble;
                esperando_nibble_bajo = true;
            } else {
                // Capturar Nibble Bajo y Ejecutar
                Byte low_nibble = data_nibble;
                Byte full_byte = (nibble_alto_actual << 4) | low_nibble;
                esperando_nibble_bajo = false;

                if (rs) {
                    // Dato (Caracter)
                    WriteCharToScreen((char)full_byte);
                } else {
                    // Comando
                    HandleCommand(full_byte);
                }
            }
        }
    }
}

void LCD::WriteCharToScreen(char c) {
    if (onChar) {
        if (c == '\n') onChar('\r');  // Convert \n to \r\n for terminal
        onChar(c);
        if (c == '\n')
            return;  // Don't print newline to LCD buffer directly as character
    }

    if (c == '\n' || c == '\r') {
        cursorX = 0;
        cursorY++;
        if (cursorY >= 2) cursorY = 0;  // Wrap to top
        return;
    }

    if (cursorX < 16 && cursorY < 2) {
        screen[cursorY][cursorX] = c;
    }

    cursorX++;
    if (cursorX >= 16) {
        cursorX = 0;
        cursorY++;
        if (cursorY >= 2) cursorY = 0;
    }
}

void LCD::HandleCommand(Byte cmd) {
    if (cmd == 0x01) {
        // Clear Display
        std::memset(screen, ' ', sizeof(screen));
        cursorX = 0;
        cursorY = 0;
    }
    // Other commands could be implemented here
}