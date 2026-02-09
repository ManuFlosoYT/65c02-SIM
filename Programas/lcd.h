#ifndef LCD_H
#define LCD_H

#define PORTB (*(volatile unsigned char*)0x6000)
#define DDRB (*(volatile unsigned char*)0x6002)

// Mapeo de pines según LCD.cpp (Bit 6=E, Bit 5=RW, Bit 4=RS)
#define E 0x40
#define RW 0x20
#define RS 0x10

/*
   Ayudante para enviar 4 bits
   modo: 0 para Instruccion, RS (0x10) para Dato
   Esta función activa el "WriteHook" en LCD.cpp
*/
void lcd_enviar_nibble(unsigned char dato, unsigned char modo) {
    dato &= 0x0F;  // Asegurar solo 4 bits bajos

    // Preparar datos y señales (Set RW=0, E=1)
    PORTB = dato | modo | E;

    // Flanco de bajada (E=1 -> E=0)
    // Esto dispara la lógica dentro de mem.SetWriteHook en LCD.cpp
    PORTB = dato | modo;
}

/*
   Envía un comando (RS=0)
*/
void lcd_instruccion(unsigned char comando) {
    lcd_enviar_nibble(comando >> 4, 0); // Nibble alto
    lcd_enviar_nibble(comando, 0);      // Nibble bajo
}

/*
   Envía un caracter (RS=1)
   Esto dispara LCD::WriteCharToScreen en el simulador
*/
void lcd_imprimir_caracter(unsigned char c) {
    lcd_enviar_nibble(c >> 4, RS);
    lcd_enviar_nibble(c, RS);
}

/*
   Dispara: LCD::HandleCommand con cmd 0x01
*/
void lcd_limpiar(void) {
    lcd_instruccion(0x01);
}

/*
   Dispara: LCD::WriteCharToScreen con '\n', lo que reinicia cursorX e incrementa cursorY
*/
void lcd_salto_linea(void) {
    lcd_imprimir_caracter('\n');
}

void lcd_inicializar(void) {
    // Configurar pines como salida
    DDRB = 0xFF;

    // Secuencia de inicializacion hardware (Protocolo HD44780)

    // 1. Function Set 8-bit (3 veces para resetear estado)
    lcd_enviar_nibble(0x03, 0);
    lcd_enviar_nibble(0x03, 0);
    lcd_enviar_nibble(0x03, 0);

    // 2. Cambiar a modo 4-bit (Detectado por LCD.cpp: if (!rs && data_nibble == 0x02))
    lcd_enviar_nibble(0x02, 0);

    // 3. Configurar Function Set: 4-bit, 2 lineas, fuente 5x8 (0x28)
    lcd_instruccion(0x28);

    // 4. Display On, Cursor On, Blink Off (0x0E)
    lcd_instruccion(0x0E);

    // 5. Entry Mode Set: Increment, No Shift (0x06)
    lcd_instruccion(0x06);

    // 6. Limpiar Display usando la nueva función
    lcd_limpiar();
}

void lcd_imprimir(const char* cadena) {
    while (*cadena) {
        lcd_imprimir_caracter(*cadena++);
    }
}

void lcd_imprimir_numero(unsigned int num) {
    unsigned char centenas, resto, decenas, unidades;

    centenas = num / 100;
    resto = num % 100;
    decenas = resto / 10;
    unidades = resto % 10;

    if (centenas > 0) {
        lcd_imprimir_caracter('0' + centenas);
        lcd_imprimir_caracter('0' + decenas);
        lcd_imprimir_caracter('0' + unidades);
    } else if (decenas > 0) {
        lcd_imprimir_caracter('0' + decenas);
        lcd_imprimir_caracter('0' + unidades);
    } else {
        lcd_imprimir_caracter('0' + unidades);
    }
}

#endif