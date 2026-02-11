#include "lcd.h"

unsigned char a;
unsigned char b;
unsigned char siguiente;

int main(void) {
    lcd_inicializar();
    lcd_imprimir("Fibonacci:\n");

    a = 0;
    b = 1;

    /* Bucle infinito hasta que desborde 8 bits. */
    while (b >= a) {
        lcd_imprimir_numero(a);
        lcd_imprimir(", ");

        siguiente = a + b;

        a = b;
        b = siguiente;
    }

    lcd_imprimir("\nUltimo: ");
    lcd_imprimir_numero(a);
    lcd_imprimir("\n");

    return 0;
}