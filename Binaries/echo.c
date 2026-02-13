#include "Include/bios.h"

#define BUFFER_SIZE 50

int main(void) {
    char inputBuffer[BUFFER_SIZE];  // Space to store user input

    int i = 0;
    for (i = 0; i < BUFFER_SIZE; i++) {
        inputBuffer[i] = 0;
    }

    /* Hardware initialization, same as in mat.c */
    INIT_BUFFER();

    /* 1. Request user input */
    print_str("Type your name (and press Enter): ");

    /* 2. Capture input */
    read_line(inputBuffer, BUFFER_SIZE);

    /* 3. Print result message */
    print_str("Hello, ");
    print_str(inputBuffer);
    print_str("! Welcome to the system.\n");

    return 0;
}