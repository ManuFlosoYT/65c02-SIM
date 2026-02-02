/* Definición de variables globales */
unsigned char i, j, k;
unsigned int suma_temp;
unsigned int suma_total;

/* Matrices diseñadas con valores bajos (0, 1, 2) para controlar el crecimiento.
   Resultado esperado de la suma total: 31 (0x1F).
*/

unsigned char matA[4][4] = {
    {1, 2, 0, 1}, 
    {0, 1, 0, 1}, 
    {1, 0, 1, 0}, 
    {0, 1, 0, 0}
};

unsigned char matB[4][4] = {
    {0, 1, 0, 1}, 
    {1, 0, 1, 0}, 
    {0, 1, 1, 1}, 
    {1, 0, 0, 0}
};

unsigned char matC[4][4] = {
    {1, 0, 1, 0}, 
    {0, 1, 0, 0}, 
    {1, 0, 0, 1}, 
    {0, 1, 1, 0}
};

/* Matrices para resultados intermedios y final */
/* Usamos int para asegurar que no haya desbordes internos, aunque con estos
   valores tan bajos, incluso char podría aguantar los pasos intermedios. */
unsigned int matTemp[4][4];
unsigned int matFinal[4][4];

int main(void) {
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

    /* 3. Sumar todos los elementos de la matriz resultante */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            suma_total += matFinal[i][j];
        }
    }

    /* 4. Escribir el resultado en 0x6767 */
    /* La suma total matemática es 31, por lo que cabe en unsigned char sin problemas */
    (*(volatile unsigned char*)0x6767) = (unsigned char)suma_total;

    return 0;
}