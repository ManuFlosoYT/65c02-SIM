#include "bios.h"

#define BUFFER_SIZE 50

int main(void) {
    char inputBuffer[BUFFER_SIZE]; // Espacio para guardar el texto del usuario

    int i = 0;
    for(i = 0; i < BUFFER_SIZE; i++) {
        inputBuffer[i] = 0;
    }

    /* Inicialización del hardware, igual que en mat.c */
    INIT_BUFFER();

    /* 1. Pedir entrada al usuario */
    print_str("Escribe tu nombre (y pulsa Enter): ");

    /* 2. Capturar la entrada */
    read_line(inputBuffer, BUFFER_SIZE);

    /* 3. Imprimir el mensaje con el resultado */
    print_str("Hola, ");
    print_str(inputBuffer);
    print_str("! Bienvenid@ al sistema.\n");

    return 0;
}