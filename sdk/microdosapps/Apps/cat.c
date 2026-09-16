/* cat.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

int main(void) {
    SD_FILE fp;
    int c;
    char** args = _args_ptr;

    if (arg_count < 2) {
        print_str("Usage: ");
        println("cat <filename>");
        return 1;
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
