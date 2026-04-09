#include "Libs/BIOS.h"

/* Global variables definition */
unsigned char i, j, k;
unsigned int sum_temp;
unsigned int sum_total;

/* Matrices designed with low values (0, 1, 2) to control growth.
   Expected result of total sum: 31 (0x1F).
*/

unsigned char matA[4][4] = {
    {1, 2, 0, 1}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 0}};

unsigned char matB[4][4] = {
    {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 1, 1}, {1, 0, 0, 0}};

unsigned char matC[4][4] = {
    {1, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 1}, {0, 1, 1, 0}};

/* Matrices for intermediate and final results */
unsigned int matTemp[4][4];
unsigned int matFinal[4][4];

int main(void) {
    INIT_BUFFER();

    sum_total = 0;

    /* 1. Multiply MatA x MatB = MatTemp */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            sum_temp = 0;
            for (k = 0; k < 4; k++) {
                sum_temp += matA[i][k] * matB[k][j];
            }
            matTemp[i][j] = sum_temp;
        }
    }

    /* 2. Multiply MatTemp x MatC = MatFinal */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            sum_temp = 0;
            for (k = 0; k < 4; k++) {
                sum_temp += matTemp[i][k] * matC[k][j];
            }
            matFinal[i][j] = sum_temp;
        }
    }

    /* Print Final Matrix */
    print_str("Resulting Matrix:\n");
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

    /* 3. Sum all elements of the resulting matrix */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            sum_total += matFinal[i][j];
        }
    }

    print_str("Total Sum: ");
    print_num(sum_total);
    print_str("\n");

    return 0;
}