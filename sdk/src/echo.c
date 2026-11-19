#include "Libs/BIOS.h"

enum EchoConfig {
    BUFFER_SIZE = 50,
};

int main(void) {
    char inputBuffer[BUFFER_SIZE] = {0};

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