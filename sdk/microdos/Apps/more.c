/* more.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

MICRODOS_NO_DESKTOP();

static bool output_char(char ch, int *lines) {
    bios_putchar(ch);
    if (ch == '\n') {
        (*lines)++;
        if (*lines >= 20) {
            print_str("--More--");
            char c;
            do {
                c = bios_getchar_nb();
            } while (c == 0);
            if (c == 'q' || c == 'Q' || c == 3) {
                return false;
            }
            print_str("\r        \r");
            *lines = 0;
        }
    }
    return true;
}

int main(void) {
    char **args = _args_ptr;
    SD_FILE fp;
    int16_t bytes_read = 0;
    uint8_t buf[64];
    int lines = 0;
    bool quit = false;
    bool from_stdin = (arg_count < 2 || strcmp(args[1], "-") == 0);

    if (from_stdin) {
        int c;
        while ((c = bios_getchar()) != 0 && c != 4 && c != 26 && c != -1) {
            if (!output_char((char)c, &lines)) {
                quit = true;
                break;
            }
        }
    } else {
        if (!sd_open(&fp, args[1], SD_READ)) {
            println("Error: File not found.");
            return 1;
        }

        while (!quit && (bytes_read = sd_read(&fp, buf, sizeof(buf))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                if (!output_char(buf[i], &lines)) {
                    quit = true;
                    break;
                }
            }
        }
        sd_close(&fp);
    }
    return 0;
}
