unsigned char a;
unsigned char b;
unsigned char siguiente;

int main(void) {
    a = 0;
    b = 1;

    /* Puntero a la dirección física 0x6767 */
    (*(volatile unsigned char*)0x6767) = a;

    /* Bucle infinito hasta que desborde 8 bits. */
    while ((unsigned char)b >= (unsigned char)a) {
        (*(volatile unsigned char*)0x6767) = b;

        siguiente = a + b;

        a = b;
        b = siguiente;
    }

    return 0;
}