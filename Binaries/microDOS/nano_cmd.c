#include "commands.h"
#include "shell.h"
#include "../Libs/BIOS.h"
#include "../Libs/SD.h"

void cmd_nano(void) {
    SD_FILE fp;
    char c;
    static char editor_buf[4096];
    int editor_len = 0;

    if (arg_count < 2) {
        println("Usage: nano <file>");
        return;
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        println("ERROR: nano failed (open)");
        return;
    }

    println("--- NANO EDITOR ---");
    println("Type your text. Press ESC to save and exit.");

    while (1) {
        c = bios_getchar();
        if (c == 27) break;

        if (c == '\b' || c == 8 || c == 127) {
            if (editor_len > 0) {
                editor_len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\r' || c == '\n') {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = '\n';
                bios_putchar('\r');
                bios_putchar('\n');
            }
        } else {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = c;
            }
        }
    }

    editor_buf[editor_len] = '\0';
    sd_puts(&fp, editor_buf);

    sd_close(&fp);
    println("");
    println("--- SAVED ---");
}
