/* clear.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>

int main(void) {
    bios_putchar('\x0C');
    return 0;
}
