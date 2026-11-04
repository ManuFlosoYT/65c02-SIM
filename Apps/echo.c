#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

int main(void) {
    char** args = _args_ptr;
    bool newline = true;
    uint8_t start_idx = 1;

    if (arg_count > 1 && strcmp(args[1], "-n") == 0) {
        newline = false;
        start_idx = 2;
    }

    for (uint8_t i = start_idx; i < arg_count; i++) {
        if (i > start_idx) {
            bios_putchar(' ');
        }
        print_str(args[i]);
    }

    if (newline) {
        bios_putchar('\r');
        bios_putchar('\n');
    }

    return 0;
}
