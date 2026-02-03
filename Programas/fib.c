#include "lcd.h"

unsigned char a;
unsigned char b;
unsigned char siguiente;

int main(void) {
    unsigned char centenas, resto, decenas, unidades;

    lcd_inicializar();

    a = 0;
    b = 1;

    /* Bucle infinito hasta que desborde 8 bits. */
    while (b >= a) {
        siguiente = a + b;

        a = b;
        b = siguiente;
    }

    centenas = a / 100;
    resto = a % 100;
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

    return 0;
}