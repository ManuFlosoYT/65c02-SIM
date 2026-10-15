#include "Libs/BIOS.h"

int main(void) {
    INIT_BUFFER();

    print_str("Starting write with color change per row...\n");

    uint8_t color = 0x00U;
    uint8_t counter = 0U;

    for (uint8_t* ptr = (uint8_t*)0x2000U; ptr <= (uint8_t*)0x3FFFU; ++ptr) {
        *ptr = color;

        counter++;

        if (counter >= 128U) {
            counter = 0U;
            color++;

            print_str("Row: ");
            print_hex((uint16_t)color);
            print_str("\n");
        }
    }

    print_str("Process finished.\n");

    return 0;
}