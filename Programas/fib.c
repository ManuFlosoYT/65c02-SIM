#define PORTB 0x6000

unsigned char a;
unsigned char b;
unsigned char siguiente;

int main(void) {
    unsigned char centenas, resto, decenas, unidades;

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
        (*(volatile unsigned char*)PORTB) = '0' + centenas;
        (*(volatile unsigned char*)PORTB) = '0' + decenas;
        (*(volatile unsigned char*)PORTB) = '0' + unidades;
    } else if (decenas > 0) {
        (*(volatile unsigned char*)PORTB) = '0' + decenas;
        (*(volatile unsigned char*)PORTB) = '0' + unidades;
    } else {
        (*(volatile unsigned char*)PORTB) = '0' + unidades;
    }

    return 0;
}