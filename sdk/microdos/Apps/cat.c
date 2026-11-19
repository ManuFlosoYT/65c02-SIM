/* cat.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

int main(void) {
    SD_FILE fp;
    int c = 0;
    char** args = _args_ptr;

    if (arg_count < 2 || strcmp(args[1], "-") == 0) {
        while ((c = bios_getchar()) != 0 && c != 4 && c != 26 && c != -1) {
            bios_putchar((char)c);
        }
        return 0;
    }
    
    if (sd_open(&fp, args[1], SD_READ)) {
        while ((c = sd_getc(&fp)) != -1) {
            bios_putchar((char)c);
        }
        println("");
        sd_close(&fp);
    } else {
        print_str("Error: Could not read file '");
        print_str(args[1]);
        println("'");
        return 1;
    }
    return 0;
}
