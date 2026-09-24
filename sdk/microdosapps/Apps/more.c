/* more.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

int main(void) {
    char **args = _args_ptr;
    SD_FILE fp;
    int16_t bytes_read;
    uint8_t buf[64];
    int lines = 0;
    int i;
    int quit = 0;

    if (arg_count < 2) {
        println("Usage: more <file>");
        return 1;
    }

    if (!sd_open(&fp, args[1], SD_READ)) {
        println("Error: File not found.");
        return 1;
    }

    while (!quit && (bytes_read = sd_read(&fp, buf, sizeof(buf))) > 0) {
        for (i = 0; i < bytes_read; i++) {
            bios_putchar(buf[i]);
            if (buf[i] == '\n') {
                lines++;
                if (lines >= 20) {
                    char c;
                    print_str("--More--");
                    c = bios_getchar();
                    if (c == 'q' || c == 'Q' || c == 3) {
                        quit = 1;
                        break;
                    }
                    /* Clear the more prompt */
                    print_str("\r        \r");
                    lines = 0;
                }
            }
        }
    }
    if (!quit) {
        println("");
    }
    sd_close(&fp);
    return 0;
}
