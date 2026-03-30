/*
 * nano.c — microDOS text editor app
 * Compile with: ./compile-bin.sh nano --microDOS
 */
#include <stdint.h>
#include <string.h>
#include "Libs/app-bios.h"

/* Args forwarded from the shell via ZP */
#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

int main(void) {
    SD_FILE fp;
    char c;
    static char buf[4096];
    int len = 0;
    char** args = _args_ptr;

    if (arg_count < 2) {
        print_str("Usage: "); println("nano <filename>");
        return 1;
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        print_str("Error: "); println("Could not open file");
        return 1;
    }

    println("NANO  ESC=save");

    while (1) {
        c = bios_getchar();
        if (c == 27) break;

        if (c == '\b' || c == 8 || c == 127) {
            if (len > 0) {
                len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\r' || c == '\n') {
            if (len < (int)(sizeof(buf) - 1)) {
                buf[len++] = '\n';
                bios_putchar('\r');
                bios_putchar('\n');
            }
        } else {
            if (len < (int)(sizeof(buf) - 1)) {
                buf[len++] = c;
            }
        }
    }

    buf[len] = '\0';
    sd_puts(&fp, buf);
    sd_close(&fp);
    println("\nSAVED");
    return 0;
}
