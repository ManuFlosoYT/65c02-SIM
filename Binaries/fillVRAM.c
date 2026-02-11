#include "bios.h"

/* Variables globales */
unsigned char* ptr;
unsigned char color;
unsigned int counter;

int main(void) {
    /* Inicializar sistema */
    INIT_BUFFER();

    print_str("Iniciando escritura con cambio de color por fila...\n");

    color = 0x00;
    counter = 0;

    /* Recorrer desde 0x2000 hasta 0x3FFF */
    for (ptr = (unsigned char*)0x2000; ptr <= (unsigned char*)0x3FFF; ptr++) {
        /* Escribir el color actual en esa dirección */
        *ptr = color;

        counter++;

        /* Cambiar color cada 128 bytes (cada fila en esquema A0-A6=X, A7-A13=Y)
         */
        if (counter >= 128) {
            counter = 0;
            color++;

            /* Imprimir indicador de cambio de fila */
            print_str("Fila: ");
            print_hex((unsigned int)color);
            print_str("\n");
        }
    }

    print_str("Fin del proceso.\n");

    return 0;
}