/* more.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

int main(void) {
    char **args = _args_ptr;
    SD_FILE fp;
    int16_t bytes_read = 0;
    uint8_t buf[64];
    int lines = 0;
    bool quit = false;

    if (arg_count < 2) {
        println("Usage: more <file>");
        return 1;
    }

    if (!sd_open(&fp, args[1], SD_READ)) {
        println("Error: File not found.");
        return 1;
    }

    while (!quit && (bytes_read = sd_read(&fp, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            bios_putchar(buf[i]);
            if (buf[i] == '\n') {
                lines++;
                if (lines >= 20) {
                    print_str("--More--");
                    char c = bios_getchar();
                    if (c == 'q' || c == 'Q' || c == 3) {
                        quit = true;
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
