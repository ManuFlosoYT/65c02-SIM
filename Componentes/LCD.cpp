#include "LCD.h"

#include <iostream>

#include "Mem.h"

void LCD::Inicializar(Mem& mem) {
    // Reiniciar estado
    modo_cuatro_bits = false;
    esperando_nibble_bajo = false;
    ultimo_portb = 0;
    DATO_PORTB = 0;
    DATO_DDRB = 0;

    mem.SetWriteHook(DDRB, [this](Word addr, Byte val) {
        DATO_DDRB = val;
        // Si estuviéramos simulando el Busy Flag, manejaríamos los bits de
        // Entrada (0) aquí. Pero dado que el código escribe 0s en los bits
        // de datos durante las verificaciones de espera, y que nuestra
        // memoria simple devuelve lo que se escribió, efectivamente
        // devolvemos 0 (No Ocupado) por defecto.
    });

    mem.SetWriteHook(PORTB, [this](Word addr, Byte val) {
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
                // Buscando específicamente el cambio a modo 4-bits
                // (Comando 0x02)
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
                        std::cout << (char)full_byte;
                        std::cout.flush();
                    } else {
                        // Comando
                        // Podríamos implementar limpiar pantalla (0x01) u otros
                        // aquí.
                    }
                }
            }
        }
    });
}