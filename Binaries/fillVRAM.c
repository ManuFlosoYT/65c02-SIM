#include "Include/BIOS.h"

/* Global variables */
unsigned char* ptr;
unsigned char color;
unsigned int counter;

int main(void) {
    /* Initialize system */
    INIT_BUFFER();

    print_str("Starting write with color change per row...\n");

    color = 0x00;
    counter = 0;

    /* Iterate from 0x2000 to 0x3FFF */
    for (ptr = (unsigned char*)0x2000; ptr <= (unsigned char*)0x3FFF; ptr++) {
        /* Write current color to address */
        *ptr = color;

        counter++;

        /* Change color every 128 bytes (each row in A0-A6=X, A7-A13=Y scheme)
         */
        if (counter >= 128) {
            counter = 0;
            color++;

            /* Print row change indicator */
            print_str("Row: ");
            print_hex((unsigned int)color);
            print_str("\n");
        }
    }

    print_str("Process finished.\n");

    return 0;
}