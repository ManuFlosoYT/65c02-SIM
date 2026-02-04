#include "bios.h"

/* Definición de variables globales */
unsigned char i, j, k;
unsigned int suma_temp;
unsigned int suma_total;

/* Matrices diseñadas con valores bajos (0, 1, 2) para controlar el crecimiento.
   Resultado esperado de la suma total: 31 (0x1F).
*/

unsigned char matA[4][4] = {
    {1, 2, 0, 1}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 0}};

unsigned char matB[4][4] = {
    {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 1, 1}, {1, 0, 0, 0}};

unsigned char matC[4][4] = {
    {1, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 1}, {0, 1, 1, 0}};

/* Matrices para resultados intermedios y final */
unsigned int matTemp[4][4];
unsigned int matFinal[4][4];

void print_str(const char* s) {
    while (*s) {
        if (*s == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
        } else {
            bios_putchar(*s);
        }
        s++;
    }
}

void print_num(unsigned int n) {
    char buffer[10];
    int idx = 0;
    if (n == 0) {
        bios_putchar('0');
        return;
    }
    while (n > 0) {
        buffer[idx++] = (n % 10) + '0';
        n /= 10;
    }
    while (idx > 0) {
        bios_putchar(buffer[--idx]);
    }
}

int main(void) {
    INIT_BUFFER();

    suma_total = 0;

    /* 1. Multiplicar MatA x MatB = MatTemp */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            suma_temp = 0;
            for (k = 0; k < 4; k++) {
                suma_temp += matA[i][k] * matB[k][j];
            }
            matTemp[i][j] = suma_temp;
        }
    }

    /* 2. Multiplicar MatTemp x MatC = MatFinal */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            suma_temp = 0;
            for (k = 0; k < 4; k++) {
                suma_temp += matTemp[i][k] * matC[k][j];
            }
            matFinal[i][j] = suma_temp;
        }
    }

    /* Imprimir Matriz Final */
    print_str("Matriz Resultante:\n");
    for (i = 0; i < 4; i++) {
        bios_putchar('[');
        for (j = 0; j < 4; j++) {
            print_num(matFinal[i][j]);
            if (j < 3) {
                print_str(", ");
            }
        }
        print_str("]\n");
    }
    print_str("\n");

    /* 3. Sumar todos los elementos de la matriz resultante */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            suma_total += matFinal[i][j];
        }
    }

    print_str("Suma Total: ");
    print_num(suma_total);
    print_str("\n");

    return 0;
}